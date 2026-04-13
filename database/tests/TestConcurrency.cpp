#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

#include "DataBaseManager.h"
#include "Repository/MessageRepository.h"
#include "Repository/UserRepository.h"
#include "schema.h"

namespace {

std::atomic<int> g_db_counter{0};

std::string uniqueDbPath() {
    auto tmp = std::filesystem::temp_directory_path();
    return (tmp / ("concur_" + std::to_string(getpid()) + "_" + std::to_string(++g_db_counter) + ".db")).string();
}

void removeDb(const std::string& path) {
    for (const char* suffix : {"", "-wal", "-shm"})
        std::filesystem::remove(path + suffix);
}

Message makeMessage(int64_t user_id, int64_t folder_id,
                    const std::string& from = "sender@test.com") {
    Message m;
    m.user_id       = user_id;
    m.folder_id     = folder_id;
    m.uid           = 0;
    m.raw_file_path = "/mnt/mail/concurrent.eml";
    m.size_bytes    = 256;
    m.from_address  = from;
    m.internal_date = "2024-01-01T00:00:00Z";
    m.is_recent     = true;
    return m;
}

} 

class ConcurrencyTest : public ::testing::Test {
protected:
    std::string                        m_path;
    std::unique_ptr<DataBaseManager>   m_mgr;

    void SetUp() override {
        m_path = uniqueDbPath();
        m_mgr = std::make_unique<DataBaseManager>(m_path, initSchema(), nullptr, 8);
        ASSERT_TRUE(m_mgr->isConnected()) << "DB failed to open: " << m_path;
    }

    void TearDown() override {
        m_mgr.reset();
        removeDb(m_path);
    }

    std::pair<UserRepository, MessageRepository> makeRepos() {
        return {UserRepository(*m_mgr), MessageRepository(*m_mgr)};
    }

    std::pair<int64_t, int64_t> setupUser(const std::string& username) {
        UserRepository    ur(*m_mgr);
        MessageRepository mr(*m_mgr);

        User u; u.username = username;
        if (!ur.registerUser(u, "pass")) return {-1, -1};

        int64_t uid = *u.id;
        int64_t fid = -1;

        auto inbox = mr.findFolderByName(uid, "INBOX");
        if (inbox) {
            fid = *inbox->id;
        } else {
            Folder f; f.user_id = uid; f.name = "INBOX";
            mr.createFolder(f);
            fid = *f.id;
        }
        return {uid, fid};
    }
};

TEST_F(ConcurrencyTest, ConcurrentUserRegistration_AllUsersCreated) {
    constexpr int N = 8;
    std::atomic<int>      success{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&, i]() {
            UserRepository ur(*m_mgr);
            User u; u.username = "reg_user_" + std::to_string(i);
            if (ur.registerUser(u, "pass")) ++success;
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(success.load(), N);

    UserRepository ur(*m_mgr);
    auto all = ur.findAll(N + 10, 0);
    EXPECT_EQ(static_cast<int>(all.size()), N);
}

TEST_F(ConcurrencyTest, ConcurrentRegistration_AssignedIDsAreUnique) {
    constexpr int N = 10;
    std::vector<int64_t>     ids;
    std::mutex               ids_mutex;
    std::vector<std::thread> threads;

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&, i]() {
            UserRepository ur(*m_mgr);
            User u; u.username = "uid_user_" + std::to_string(i);
            if (ur.registerUser(u, "pass") && u.id) {
                std::lock_guard<std::mutex> lk(ids_mutex);
                ids.push_back(*u.id);
            }
        });
    }
    for (auto& t : threads) t.join();

    ASSERT_EQ(static_cast<int>(ids.size()), N);
    std::sort(ids.begin(), ids.end());
    EXPECT_EQ(std::adjacent_find(ids.begin(), ids.end()), ids.end())
        << "Duplicate user IDs detected under concurrent registration";
}


TEST_F(ConcurrencyTest, ConcurrentReads_AllReadersGetConsistentCount) {
    {
        UserRepository ur(*m_mgr);
        for (int i = 0; i < 5; ++i) {
            User u; u.username = "read_user_" + std::to_string(i);
            ur.registerUser(u, "pass");
        }
    }

    constexpr int READERS = 8;
    std::atomic<int>      ok{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < READERS; ++i) {
        threads.emplace_back([&]() {
            UserRepository ur(*m_mgr);
            auto users = ur.findAll(50, 0);
            if (users.size() == 5u) ++ok;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(ok.load(), READERS);
}

TEST_F(ConcurrencyTest, ConcurrentFolderLookups_AllSeeCorrectFolders) {
    auto [uid, fid] = setupUser("folder_reader");
    ASSERT_GT(uid, 0);

    {
        MessageRepository mr(*m_mgr);
        for (int i = 0; i < 4; ++i) {
            Folder f; f.user_id = uid; f.name = "Folder_" + std::to_string(i);
            mr.createFolder(f);
        }
    }

    constexpr int READERS = 6;
    std::atomic<int>      ok{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < READERS; ++i) {
        threads.emplace_back([&]() {
            MessageRepository mr(*m_mgr);
            auto folders = mr.findFoldersByUser(uid);
            // At least the 4 we created (plus possibly INBOX)
            if (folders.size() >= 4u) ++ok;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(ok.load(), READERS);
}

TEST_F(ConcurrencyTest, ConcurrentDelivery_NoMessagesLost) {
    auto [uid, fid] = setupUser("delivery_user");
    ASSERT_GT(uid, 0);

    constexpr int WRITERS          = 4;
    constexpr int MSGS_PER_WRITER  = 10;
    std::atomic<int>      written{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < WRITERS; ++t) {
        threads.emplace_back([&, t]() {
            MessageRepository mr(*m_mgr);
            for (int j = 0; j < MSGS_PER_WRITER; ++j) {
                Message m = makeMessage(uid, fid,
                    "writer" + std::to_string(t) + "@test.com");
                if (mr.deliver(m, fid)) ++written;
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(written.load(), WRITERS * MSGS_PER_WRITER);

    MessageRepository mr(*m_mgr);
    auto msgs = mr.findByFolder(fid, WRITERS * MSGS_PER_WRITER + 10, 0);
    EXPECT_EQ(static_cast<int>(msgs.size()), WRITERS * MSGS_PER_WRITER);
}

TEST_F(ConcurrencyTest, ConcurrentDelivery_UIDs_AreUniqueWithinFolder) {
    auto [uid, fid] = setupUser("uid_user");
    ASSERT_GT(uid, 0);

    constexpr int WRITERS = 4, MSGS = 5;
    std::vector<int64_t>     uids;
    std::mutex               uids_mutex;
    std::vector<std::thread> threads;

    for (int t = 0; t < WRITERS; ++t) {
        threads.emplace_back([&]() {
            MessageRepository mr(*m_mgr);
            for (int j = 0; j < MSGS; ++j) {
                Message m = makeMessage(uid, fid);
                if (mr.deliver(m, fid)) {
                    std::lock_guard<std::mutex> lk(uids_mutex);
                    uids.push_back(m.uid);
                }
            }
        });
    }
    for (auto& t : threads) t.join();

    ASSERT_EQ(static_cast<int>(uids.size()), WRITERS * MSGS);
    std::sort(uids.begin(), uids.end());
    EXPECT_EQ(std::adjacent_find(uids.begin(), uids.end()), uids.end())
        << "Duplicate UIDs detected under concurrent delivery";
}

TEST_F(ConcurrencyTest, ConcurrentFlagUpdates_NoDeadlockOrCorruption) {
    auto [uid, fid] = setupUser("flag_user");
    ASSERT_GT(uid, 0);

    constexpr int NUM_MSGS = 10;
    std::vector<int64_t> msg_ids;
    {
        MessageRepository mr(*m_mgr);
        for (int i = 0; i < NUM_MSGS; ++i) {
            Message m = makeMessage(uid, fid);
            ASSERT_TRUE(mr.deliver(m, fid));
            msg_ids.push_back(*m.id);
        }
    }

    constexpr int THREADS = 4;
    std::atomic<int>      done{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&, t]() {
            MessageRepository mr(*m_mgr);
            for (auto id : msg_ids) {
                mr.markSeen(id,    t % 2 == 0);
                mr.markFlagged(id, t % 2 != 0);
                mr.markDeleted(id, false);       
            }
            ++done;
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_EQ(done.load(), THREADS);

    MessageRepository mr(*m_mgr);
    for (auto id : msg_ids)
        EXPECT_TRUE(mr.findByID(id).has_value()) << "Message " << id << " lost";
}

TEST_F(ConcurrencyTest, MixedReadWrite_ReadersNeverSeeMoreThanWritten) {
    auto [uid, fid] = setupUser("rw_user");
    ASSERT_GT(uid, 0);

    constexpr int WRITERS = 3, MSGS_PER_WRITER = 8, READERS = 4;
    std::atomic<int>      write_ok{0};
    std::atomic<int>      read_ok{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < WRITERS; ++t) {
        threads.emplace_back([&]() {
            MessageRepository mr(*m_mgr);
            for (int j = 0; j < MSGS_PER_WRITER; ++j) {
                Message m = makeMessage(uid, fid);
                if (mr.deliver(m, fid)) ++write_ok;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    for (int t = 0; t < READERS; ++t) {
        threads.emplace_back([&]() {
            MessageRepository mr(*m_mgr);
            for (int r = 0; r < 5; ++r) {
                auto msgs = mr.findByFolder(fid, 1000, 0);
                if (static_cast<int>(msgs.size()) <= WRITERS * MSGS_PER_WRITER)
                    ++read_ok;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        });
    }

    for (auto& th : threads) th.join();

    EXPECT_EQ(write_ok.load(), WRITERS * MSGS_PER_WRITER);
    EXPECT_EQ(read_ok.load(),  READERS * 5);

    MessageRepository mr(*m_mgr);
    auto final_msgs = mr.findByFolder(fid, 1000, 0);
    EXPECT_EQ(static_cast<int>(final_msgs.size()), WRITERS * MSGS_PER_WRITER);
}

TEST_F(ConcurrencyTest, MixedReadWrite_ConcurrentFolderCreationAndLookup) {
    auto [uid, _fid] = setupUser("folder_rw_user");
    ASSERT_GT(uid, 0);

    constexpr int CREATORS = 4, READERS = 4, FOLDERS_EACH = 3;
    std::atomic<int>      create_ok{0};
    std::atomic<int>      read_ok{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < CREATORS; ++t) {
        threads.emplace_back([&, t]() {
            MessageRepository mr(*m_mgr);
            for (int i = 0; i < FOLDERS_EACH; ++i) {
                Folder f;
                f.user_id = uid;
                f.name    = "T" + std::to_string(t) + "_F" + std::to_string(i);
                if (mr.createFolder(f)) ++create_ok;
            }
        });
    }

    for (int t = 0; t < READERS; ++t) {
        threads.emplace_back([&]() {
            MessageRepository mr(*m_mgr);
            for (int r = 0; r < 5; ++r) {
                auto folders = mr.findFoldersByUser(uid, 100, 0);
                if (static_cast<int>(folders.size()) <= CREATORS * FOLDERS_EACH + 1 /*INBOX*/)
                    ++read_ok;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    for (auto& th : threads) th.join();
    EXPECT_EQ(create_ok.load(), CREATORS * FOLDERS_EACH);
    EXPECT_EQ(read_ok.load(),   READERS  * 5);
}

TEST_F(ConcurrencyTest, ConnectionPool_MoreReadersThanPoolSlots_AllComplete) {
    {
        UserRepository ur(*m_mgr);
        User u; u.username = "pool_user";
        ur.registerUser(u, "pass");
    }

    constexpr int READERS = 16;
    std::atomic<int>      completed{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < READERS; ++i) {
        threads.emplace_back([&]() {
            UserRepository ur(*m_mgr);
            for (int r = 0; r < 3; ++r) {
                auto users = ur.findAll(50, 0);
                EXPECT_FALSE(users.empty());
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            ++completed;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(completed.load(), READERS);
}

TEST_F(ConcurrencyTest, StressTest_MixedOps_NoDeadlock) {
    constexpr int USERS   = 4;
    constexpr int THREADS = 8;
    constexpr int OPS     = 15;

    std::vector<int64_t> user_ids, folder_ids;
    {
        UserRepository    ur(*m_mgr);
        MessageRepository mr(*m_mgr);
        for (int i = 0; i < USERS; ++i) {
            User u; u.username = "stress_" + std::to_string(i);
            ASSERT_TRUE(ur.registerUser(u, "pass"));
            user_ids.push_back(*u.id);

            auto inbox = mr.findFolderByName(*u.id, "INBOX");
            if (inbox) {
                folder_ids.push_back(*inbox->id);
            } else {
                Folder f; f.user_id = *u.id; f.name = "INBOX";
                ASSERT_TRUE(mr.createFolder(f));
                folder_ids.push_back(*f.id);
            }
        }
    }

    std::atomic<int>      ops_done{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&, t]() {
            UserRepository    ur(*m_mgr);
            MessageRepository mr(*m_mgr);

            for (int op = 0; op < OPS; ++op) {
                int idx = (t * OPS + op) % USERS;
                int64_t uid = user_ids[idx];
                int64_t fid = folder_ids[idx];

                switch (op % 5) {
                    case 0:
                        ur.findByID(uid);
                        break;
                    case 1: {
                        Message m = makeMessage(uid, fid);
                        mr.deliver(m, fid);
                        break;
                    }
                    case 2:
                        mr.findByFolder(fid, 20, 0);
                        break;
                    case 3: {
                        auto msgs = mr.findByFolder(fid, 1, 0);
                        if (!msgs.empty() && msgs[0].id)
                            mr.markSeen(*msgs[0].id, true);
                        break;
                    }
                    case 4:
                        mr.findUnseen(fid, 10, 0);
                        break;
                }
                ++ops_done;
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(ops_done.load(), THREADS * OPS)
        << "Not all operations completed — possible deadlock or crash";
}


TEST_F(ConcurrencyTest, ConcurrentExpunge_NoPhantomRows) {
    auto [uid, fid] = setupUser("expunge_user");
    ASSERT_GT(uid, 0);

    constexpr int BATCH = 20;
    {
        MessageRepository mr(*m_mgr);
        for (int i = 0; i < BATCH; ++i) {
            Message m = makeMessage(uid, fid);
            ASSERT_TRUE(mr.deliver(m, fid));
            mr.markDeleted(*m.id, true);
        }
    }

    constexpr int EXPUNGERS = 4;
    std::atomic<int> expunge_ok{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < EXPUNGERS; ++t) {
        threads.emplace_back([&]() {
            MessageRepository mr(*m_mgr);
            if (mr.expunge(fid)) ++expunge_ok;
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_GE(expunge_ok.load(), 1);

    MessageRepository mr(*m_mgr);
    auto deleted = mr.findDeleted(fid, 100, 0);
    EXPECT_TRUE(deleted.empty())
        << deleted.size() << " deleted messages still present after concurrent expunge";
}