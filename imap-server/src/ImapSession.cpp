#include "ImapSession.hpp"

#include <sstream>

#include "Entity/Folder.h"
#include "Entity/User.h"
#include "ImapParser.hpp"
#include "ImapResponse.hpp"
#include "ImapUtils.hpp"

using namespace ImapResponse;

ImapSession::ImapSession(boost::asio::ip::tcp::socket socket, Logger& logger, DataBaseManager& db, UserDAL& u_dal)
	: m_socket(std::move(socket)), m_logger(logger), m_mess_repo(db), m_user_repo(u_dal)
{
	m_logger.Log(PROD, "New ImapSession created");
	m_logger.Log(TRACE, "ImapSession::ImapSession - socket accepted");
	m_handlers = {
		{ImapCommandType::Login, [this](const ImapCommand& cmd) { return HandleLogin(cmd); }},
		{ImapCommandType::Logout, [this](const ImapCommand& cmd) { return HandleLogout(cmd); }},
		{ImapCommandType::Capability, [this](const ImapCommand& cmd) { return HandleCapability(cmd); }},
		{ImapCommandType::Noop, [this](const ImapCommand& cmd) { return HandleNoop(cmd); }},
		{ImapCommandType::Select, [this](const ImapCommand& cmd) { return HandleSelect(cmd); }},
		{ImapCommandType::List, [this](const ImapCommand& cmd) { return HandleList(cmd); }},
		{ImapCommandType::Lsub, [this](const ImapCommand& cmd) { return HandleLsub(cmd); }},
		{ImapCommandType::Status, [this](const ImapCommand& cmd) { return HandleStatus(cmd); }},
		{ImapCommandType::Fetch, [this](const ImapCommand& cmd) { return HandleFetch(cmd); }},
		{ImapCommandType::Store, [this](const ImapCommand& cmd) { return HandleStore(cmd); }},
		{ImapCommandType::Create, [this](const ImapCommand& cmd) { return HandleCreate(cmd); }},
		{ImapCommandType::Delete, [this](const ImapCommand& cmd) { return HandleDelete(cmd); }},
		{ImapCommandType::Rename, [this](const ImapCommand& cmd) { return HandleRename(cmd); }},
		{ImapCommandType::Copy, [this](const ImapCommand& cmd) { return HandleCopy(cmd); }},
		{ImapCommandType::Expunge, [this](const ImapCommand& cmd) { return HandleExpunge(cmd); }},
	};
}

void ImapSession::Start()
{
	m_logger.Log(DEBUG, "ImapSession::Start - Start");
	SendBanner();
	m_logger.Log(DEBUG, "ImapSession::Start - End");
}

void ImapSession::SendBanner()
{
	m_logger.Log(DEBUG, "ImapSession::SendBanner - Start");
	WriteResponse("* OK IMAP Server Ready\r\n");
	m_logger.Log(PROD, "ImapSession::SendBanner - Banner sent");
	m_logger.Log(DEBUG, "ImapSession::SendBanner - End");
	ReadCommand();
}

void ImapSession::ReadCommand()
{
	m_logger.Log(DEBUG, "ImapSession::ReadCommand - Start");
	auto self = shared_from_this();
	boost::asio::async_read_until(m_socket, m_buffer, "\r\n",
								  [this, self](std::error_code ec, std::size_t)
								  {
									  if (!ec)
									  {
										  std::istream is(&m_buffer);
										  std::string line;
										  std::getline(is, line);
										  if (!line.empty() && line.back() == '\r')
										  {
											  line.pop_back();
										  }
										  m_logger.Log(DEBUG, "ImapSession::ReadCommand - Acquired: " + line);
										  m_logger.Log(TRACE, "ImapSession::ReadCommand - Calling HandleCommand");
										  HandleCommand(line);
									  }
									  else
									  {
										  m_logger.Log(PROD, "ImapSession::ReadCommand - Error: " + ec.message());
										  m_socket.close();
									  }
									  m_logger.Log(DEBUG, "ImapSession::ReadCommand - End");
								  });
}

bool ImapSession::RequiresAuth(ImapCommandType type) const
{
	m_logger.Log(TRACE, "ImapSession::RequiresAuth - In: type=" + IMAP_UTILS::CommandTypeToString(type));
	m_logger.Log(DEBUG, "ImapSession::RequiresAuth - Start");

	bool result;
	switch (type)
	{
	case ImapCommandType::Login:
	case ImapCommandType::Logout:
	case ImapCommandType::Capability:
	case ImapCommandType::Noop:
		result = false;
		break;
	default:
		result = true;
		break;
	}

	m_logger.Log(TRACE, "ImapSession::RequiresAuth - Out: " + std::string(result ? "true" : "false"));
	m_logger.Log(DEBUG, "ImapSession::RequiresAuth - End");
	return result;
}

void ImapSession::HandleCommand(const std::string& line)
{
	m_logger.Log(TRACE, "ImapSession::HandleCommand - In: line=" + line);
	m_logger.Log(DEBUG, "ImapSession::HandleCommand - Start");

	auto cmd = ImapParser::Parse(line);

	if (cmd.m_type == ImapCommandType::Unknown)
	{
		m_logger.Log(PROD, "ImapSession::HandleCommand - Unknown command");
		std::string response = cmd.m_tag + " BAD Command not recognized\r\n";
		WriteResponse(response);
		m_logger.Log(TRACE, "ImapSession::HandleCommand - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleCommand - End");
		ReadCommand();
		return;
	}

	if (RequiresAuth(cmd.m_type) && m_state == SessionState::NonAuthenticated)
	{
		m_logger.Log(PROD, "ImapSession::HandleCommand - User not authenticated");
		std::string response = cmd.m_tag + " BAD Not authenticated\r\n";
		WriteResponse(response);
		m_logger.Log(TRACE, "ImapSession::HandleCommand - Out: " + response);
		m_logger.Log(DEBUG, "ImapSession::HandleCommand - End");
		ReadCommand();
		return;
	}

	auto it = m_handlers.find(cmd.m_type);
	if (it != m_handlers.end())
	{
		auto response = it->second(cmd);
		WriteResponse(response);
		m_logger.Log(TRACE, "ImapSession::HandleCommand - Out: " + response);
	}
	else
	{
		m_logger.Log(PROD, "ImapSession::HandleCommand - Command not implemented");
		std::string response = cmd.m_tag + " BAD Command not implemented\r\n";
		WriteResponse(response);
		m_logger.Log(TRACE, "ImapSession::HandleCommand - Out: " + response);
	}

	m_logger.Log(DEBUG, "ImapSession::HandleCommand - End");

	if (cmd.m_type != ImapCommandType::Logout)
	{
		ReadCommand();
	}
}

void ImapSession::WriteResponse(const std::string& msg)
{
	m_logger.Log(TRACE, "ImapSession::WriteResponse - In: msg length=" + std::to_string(msg.size()));
	m_logger.Log(DEBUG, "ImapSession::WriteResponse - Start");

	auto self = shared_from_this();
	boost::asio::async_write(m_socket, boost::asio::buffer(msg),
							 [this, self](std::error_code ec, std::size_t bytes_transferred)
							 {
								 if (ec)
								 {
									 m_logger.Log(PROD, "ImapSession::WriteResponse - Error: " + ec.message());
								 }
								 else
								 {
									 m_logger.Log(DEBUG, "ImapSession::WriteResponse - Sent " +
															 std::to_string(bytes_transferred) + " bytes");
								 }
								 m_logger.Log(DEBUG, "ImapSession::WriteResponse - End");
							 });
}

// Command

std::string ImapSession::HandleLogin(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleLogin - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleLogin - Start");

	std::string response;
	if (cmd.m_args.size() != 2)
	{
		response = cmd.m_tag + " BAD Missing arguments\r\n";
		return response;
	}
	else if (m_user_repo.authorize(cmd.m_args[0], cmd.m_args[1]))
	{
		m_authenticatedUserID = m_user_repo.findByUsername(cmd.m_args[0])->id;

		if (m_authenticatedUserID.has_value())
		{
			m_logger.Log(PROD, "ImapSession::HandleLogin - Authenticated user ID: " +
								   std::to_string(m_authenticatedUserID.value()));
			m_logger.Log(PROD, "ImapSession::HandleLogin - User authenticated: " + m_authenticatedUserName);
			response = cmd.m_tag + " OK Login successful\r\n";
			m_state = SessionState::Authenticated;
			m_authenticatedUserName = cmd.m_args[0];
		}
		else
		{
			m_logger.Log(PROD, "ImapSession::HandleLogin - Database inconsistency: Authenticated user ID not found");
			response = cmd.m_tag + " NO Login failed\r\n";
		}
	}
	else
	{
		response = cmd.m_tag + " NO Login failed\r\n";
		m_logger.Log(PROD, "ImapSession::HandleLogin - Login failed, error: " + m_user_repo.getLastError());
	}

	m_logger.Log(TRACE, "ImapSession::HandleLogin - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleLogin - End");
	return response;
}

std::string ImapSession::HandleLogout(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleLogout - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleLogout - Start");

	m_state = SessionState::NonAuthenticated;
	m_authenticatedUserName.clear();
	m_authenticatedUserID.reset();
	m_currentMailbox = {};

	std::string response = "* BYE Logout completed\r\n" + cmd.m_tag + " OK Logout completed\r\n";
	m_logger.Log(PROD, "ImapSession::HandleLogout - User logged out");
	m_logger.Log(TRACE, "ImapSession::HandleLogout - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleLogout - End");
	return response;
}

std::string ImapSession::HandleCapability(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleCapability - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleCapability - Start");

	std::string response = ImapResponse::Capability() + cmd.m_tag + " OK Capability completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleCapability - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleCapability - End");
	return response;
}

std::string ImapSession::HandleNoop(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleNoop - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleNoop - Start");

	std::string response = cmd.m_tag + " OK Noop completed\r\n";

	m_logger.Log(TRACE, "ImapSession::HandleNoop - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleNoop - End");
	return response;
}

std::string ImapSession::HandleSelect(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleSelect - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleSelect - Start");

	std::string response;
	if (cmd.m_args.size() != 1)
	{
		std::string response = cmd.m_tag + " BAD Invalid arguments number\r\n";
	}
	else
	{
		auto folder_opt = m_mess_repo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[0]);
		if (folder_opt.has_value())
		{
			m_currentMailbox.m_name = folder_opt->name;
			auto all_messages = m_mess_repo.findByFolder(folder_opt->id.value());
			m_currentMailbox.m_exists = all_messages.size();
			// recent is not implemented yet, sp it copies the unseen count for now
			m_currentMailbox.m_recent = std::count_if(all_messages.begin(), all_messages.end(),
													  [](const Message& msg) { return !msg.is_seen; });
			m_currentMailbox.m_id = folder_opt->id;

			response = ImapResponse::Flags() + ImapResponse::Exists(m_currentMailbox.m_exists) +
					   ImapResponse::Recent(m_currentMailbox.m_recent);
			response += cmd.m_tag + " OK [READ-WRITE] Select completed\r\n";
			m_state = SessionState::Selected;
			m_logger.Log(PROD, "ImapSession::HandleSelect - Mailbox selected: " + m_currentMailbox.m_name);
		}
		else
		{
			response = cmd.m_tag + " BAD Mailbox not found\r\n";
			m_logger.Log(PROD, "ImapSession::HandleSelect - Mailbox not found: " + cmd.m_args[0]);
		}
	}

	m_logger.Log(TRACE, "ImapSession::HandleSelect - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleSelect - End");
	return response;
}

std::string ImapSession::HandleList(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleList - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleList - Start");

	std::string response = "";

	if (cmd.m_args.size() != 2)
	{
		response = cmd.m_tag + " BAD Invalid arguments number\r\n";
	}
	else if (cmd.m_args[1] == "")
	{
		response = "* LIST (\\Noselect) \"/\" \"\"\r\n";
	}
	else
	{
		// In schema we didn`t implment hierarchy,
		// neither with parent_id nor with delimiter(no functions in repo to support it),
		// so we ignore the reference and return all folders for the user

		auto folders = m_mess_repo.findFoldersByUser(m_authenticatedUserID.value());
		for (const auto& folder : folders)
		{
			response += "* LIST (\\HasNoChildren) \"/\" \"" + folder.name + "\"\r\n";
		}
		response += cmd.m_tag + " OK List completed\r\n";
	}

	m_logger.Log(TRACE, "ImapSession::HandleList - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleList - End");
	return response;
}

std::string ImapSession::HandleLsub(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleLsub - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleLsub - Start");

	std::string response = "";

	if (cmd.m_args.size() != 2)
	{
		response = cmd.m_tag + " BAD Invalid arguments number\r\n";
	}
	else if (cmd.m_args[1] == "")
	{
		response = "* LSUB (\\Noselect) \"/\" \"\"\r\n";
	}
	else
	{
		// other than issues described in HandleList, schema doesn`t support subscribed folders,
		// so we return all folders for the user with \HasNoChildren flag

		auto folders = m_mess_repo.findFoldersByUser(m_authenticatedUserID.value());
		for (const auto& folder : folders)
		{
			response += "* LSUB (\\HasNoChildren) \"/\" \"" + folder.name + "\"\r\n";
		}
		response += cmd.m_tag + " OK Lsub completed\r\n";
	}

	m_logger.Log(TRACE, "ImapSession::HandleLsub - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleLsub - End");
	return response;
}

std::string ImapSession::HandleStatus(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleStatus - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleStatus - Start");

	std::string response = "";
	if (cmd.m_args.size() != 2)
	{
		response = cmd.m_tag + " BAD Invalid arguments number\r\n";
	}
	else
	{
		auto folder_opt = m_mess_repo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[0]);
		if (folder_opt.has_value())
		{
			auto all_messages = m_mess_repo.findByFolder(folder_opt->id.value());
			auto reqs = IMAP_UTILS::SplitArgs(IMAP_UTILS::TrimParentheses(cmd.m_args[1]));
			bool success = false;

			for (auto& req : reqs)
			{
				if (req == "MESSAGES")
				{
					success = true;
					response += "MESSAGES " + std::to_string(all_messages.size()) + " ";
				}
				else if (req == "RECENT") // recent is not implemented yet,
				{						  // so we return the count of unseen messages for now
					success = true;
					size_t recent = std::count_if(all_messages.begin(), all_messages.end(),
												  [](const Message& msg) { return !msg.is_seen; });
					response += "RECENT " + std::to_string(recent) + " ";
				}
				else if (req == "UNSEEN")
				{
					success = true;
					size_t unseen = std::count_if(all_messages.begin(), all_messages.end(),
												  [](const Message& msg) { return !msg.is_seen; });
					response += "UNSEEN " + std::to_string(unseen) + " ";
				}
			}

			if (success)
			{
				response = ImapResponse::Status(cmd.m_args[0], response);
				response += cmd.m_tag + " OK Status completed\r\n";
			}
			else
			{
				response = cmd.m_tag + " BAD Invalid status data items\r\n";
				m_logger.Log(PROD, "ImapSession::HandleStatus - Invalid status data items: " + cmd.m_args[1]);
			}
		}
		else
		{
			response = cmd.m_tag + " BAD Mailbox not found\r\n";
			m_logger.Log(PROD, "ImapSession::HandleStatus - Mailbox not found: " + cmd.m_args[0]);
		}
	}

	m_logger.Log(TRACE, "ImapSession::HandleStatus - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleStatus - End");
	return response;
}

std::string ImapSession::HandleFetch(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleFetch - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleFetch - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		std::string response = cmd.m_tag + " BAD No mailbox selected\r\n";
	}
	else if (cmd.m_args.size() < 2)
	{
		std::string response = cmd.m_tag + " BAD Missing arguments\r\n";
	}
	else
	{
		try
		{
			auto messages = m_mess_repo.findByFolder(m_currentMailbox.m_id.value());
			IMAP_UTILS::SortMessagesByTimeDescending(messages);
			auto lists_ids = IMAP_UTILS::ParseSequenceSet(cmd.m_args[0], messages.size());

			std::vector<Message> selected_messages;
			selected_messages.reserve(lists_ids.size());

			for (int seq_num : lists_ids)
			{
				if (seq_num > 0 && seq_num <= static_cast<int>(messages.size()))
				{
					// seq_num is coming from IMAP so it is 1-based, while vector is 0-based, so we need to subtract 1
					selected_messages.push_back(messages[seq_num - 1]);
				}
			}

			auto data_items_str = IMAP_UTILS::TrimParentheses(cmd.m_args[1]);
			auto data_items = IMAP_UTILS::SplitArgs(data_items_str);

			// expending macros
			std::vector<std::string> expanded_items;
			for (const auto& item : data_items)
			{
				std::string upper_item = IMAP_UTILS::ToUpper(item);
				if (upper_item == "ALL")
				{
					expanded_items.push_back("FLAGS");
					expanded_items.push_back("INTERNALDATE");
					expanded_items.push_back("RFC822.SIZE");
					expanded_items.push_back("ENVELOPE");
				}
				else if (upper_item == "FAST")
				{
					expanded_items.push_back("FLAGS");
					expanded_items.push_back("INTERNALDATE");
					expanded_items.push_back("RFC822.SIZE");
				}
				else if (upper_item == "FULL")
				{
					expanded_items.push_back("FLAGS");
					expanded_items.push_back("INTERNALDATE");
					expanded_items.push_back("RFC822.SIZE");
					expanded_items.push_back("ENVELOPE");
					expanded_items.push_back("BODY");
				}
				else
				{
					expanded_items.push_back(upper_item);
				}
			}

			for (int i = 0; i < selected_messages.size(); ++i)
			{
				const auto& msg = selected_messages[i];
				int seq_num = lists_ids[i];

				std::string fetch_response = "(";
				for (const auto& item : expanded_items)
				{
					if (item == "FLAGS")
					{
						fetch_response += "FLAGS (";
						if (msg.is_seen) fetch_response += "\\Seen ";
						if (msg.is_starred) fetch_response += "\\Flagged ";
						if (msg.is_important) fetch_response += "\\Important ";
						fetch_response += ") ";
					}
					else if (item == "INTERNALDATE") // db uses ISO format
					{
						fetch_response += "INTERNALDATE \"" + msg.created_at + "\" ";
					}
					else if (item == "RFC822.SIZE")
					{
						fetch_response += "RFC822.SIZE " + std::to_string(msg.body.size()) + " ";
					}
					else if (item == "ENVELOPE")
					{
						// realization for curent schema before DEMO #1
						fetch_response += "ENVELOPE (\"";
						fetch_response += "\"" + msg.created_at + "\" \"" + msg.subject + "\" \"" +
										  m_user_repo.findByID(msg.user_id)->username + "\" " + "NIL NIL";
						fetch_response +=
							"\"" + msg.receiver + "\" NIL NIL NIL NIL \"" + std::to_string(msg.id.value()) + "\") ";
					}
					else if (item == "BODY" || item == "RFC822")
					{
						fetch_response += "BODY {" + std::to_string(msg.body.size()) + "}\r\n" + msg.body + " ";
					}
					else if (item == "RFC822.TEXT")
					{
						fetch_response += "RFC822.TEXT {" + std::to_string(msg.body.size()) + "}\r\n" + msg.body + " ";
					}
					else if (item == "RFC822.HEADER")
					{
						std::string header = "Subject: " + msg.subject + "\r\nTo: " + msg.receiver + "\r\n";
						fetch_response += "RFC822.HEADER {" + std::to_string(header.size()) + "}\r\n" + header + " ";
					}
					else if (item == "BODYSTRUCTURE") // FIX: integrate MIME properly
					{
						fetch_response += "BODYSTRUCTURE (\"text\" \"plain\" NIL NIL NIL \"7BIT\" " +
										  std::to_string(msg.body.size()) + " 0) ";
					}
					// MIME parts are not implemented yet, so we ignore BODY[] and BODY.PEEK[] for now
				}
				fetch_response += ")";

				response += ImapResponse::Fetch(seq_num, fetch_response) + "\r\n";
			}

			response += cmd.m_tag + " OK Fetch completed\r\n";
		}
		catch (const std::exception& ex)
		{
			response = cmd.m_tag + " BAD Invalid message sequence\r\n";
			m_logger.Log(PROD, "ImapSession::HandleFetch - Invalid FETCH usage, exception: " + std::string(ex.what()));
			m_logger.Log(TRACE, "ImapSession::HandleFetch - Out: " + response);
			m_logger.Log(DEBUG, "ImapSession::HandleFetch - End");
			return response;
		}
	}

	m_logger.Log(TRACE, "ImapSession::HandleFetch - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleFetch - End");
	return response;
}

std::string ImapSession::HandleStore(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleStore - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleStore - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = cmd.m_tag + " BAD No mailbox selected\r\n";
	}
	else if (cmd.m_args.size() != 3)
	{
		response = cmd.m_tag + " BAD Invalid arguments number\r\n";
	}
	else
	{
		try
		{
			auto messages = m_mess_repo.findByFolder(m_currentMailbox.m_id.value());
			IMAP_UTILS::SortMessagesByTimeDescending(messages);
			auto lists_ids = IMAP_UTILS::ParseSequenceSet(cmd.m_args[0], messages.size());

			std::vector<Message> selected_messages;
			selected_messages.reserve(lists_ids.size());

			for (int seq_num : lists_ids)
			{
				if (seq_num > 0 && seq_num <= static_cast<int>(messages.size()))
				{
					// seq_num is coming from IMAP so it is 1-based, while vector is 0-based, so we need to subtract 1
					selected_messages.push_back(messages[seq_num - 1]);
				}
			}

			auto flags = IMAP_UTILS::SplitArgs(IMAP_UTILS::TrimParentheses(cmd.m_args[2]));
			bool silence = cmd.m_args[1].find(".SILENT") != std::string::npos;
			if (cmd.m_args[1][0] == '+' || cmd.m_args[1][0] == '-')
			{
				for (int i{0}; i < selected_messages.size(); i++)
				{
					for (const auto& flag : flags)
					{
						if (flag == "\\Seen")
						{
							m_mess_repo.markSeen(selected_messages[i].id.value(), cmd.m_args[1][0] == '+');
						}
						else if (flag == "\\Flagged")
						{
							// in repo flagged is ascocieted with 'starred' name
							m_mess_repo.markStarred(selected_messages[i].id.value(), cmd.m_args[1][0] == '+');
						}
						else if (flag == "\\Important")
						{
							// not standart RFC protocol
							// either our design decision or simply mistake for taking 'starred'(flagged) as important
							m_mess_repo.markImportant(selected_messages[i].id.value(), cmd.m_args[1][0] == '+');
						}
					}
					if (!silence)
					{
						response += ImapResponse::Untagged(std::to_string(lists_ids[i]) + " FETCH (FLAGS (" +
														   (selected_messages[i].is_seen ? "\\Seen " : "") +
														   (selected_messages[i].is_starred ? "\\Flagged " : "") +
														   (selected_messages[i].is_important ? "\\Important " : "") +
														   "))\r\n");
					}
				}
			}
			else
			{
				// repo doesn`t support setting flags directly, so we ignore this case for now
			}
		}
		catch (const std::exception& ex)
		{
			response = cmd.m_tag + " BAD Invalid message sequence\r\n";
			m_logger.Log(PROD, "ImapSession::HandleStore - Invalid STORE usage, exception: " + std::string(ex.what()));
			m_logger.Log(TRACE, "ImapSession::HandleStore - Out: " + response);
			m_logger.Log(DEBUG, "ImapSession::HandleStore - End");
			return response;
		}

		response = cmd.m_tag + " OK Store completed\r\n";
	}

	m_logger.Log(TRACE, "ImapSession::HandleStore - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleStore - End");
	return response;
}

std::string ImapSession::HandleCreate(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleCreate - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleCreate - Start");

	std::string response = "";
	if (cmd.m_args.size() != 1)
	{
		response = cmd.m_tag + " BAD Invalid parametrs number\r\n";
	}
	else
	{
		// with name like parent/child doesn`t handle well, because we don`t support hierarchy in schema,
		// so we just create folder with such name without parent-child relation

		Folder f{std::nullopt, m_authenticatedUserID.value(), cmd.m_args[0]};
		m_mess_repo.createFolder(f);
		response = cmd.m_tag + " OK Create completed\r\n";
	}

	m_logger.Log(TRACE, "ImapSession::HandleCreate - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleCreate - End");
	return response;
}

std::string ImapSession::HandleDelete(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleDelete - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleDelete - Start");

	std::string response = "";
	if (cmd.m_args.size() != 1)
	{
		response = cmd.m_tag + " BAD Invalid parameters number\r\n";
	}
	else
	{
		auto folder_opt = m_mess_repo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[0]);
		if (!folder_opt.has_value())
		{
			response = cmd.m_tag + " BAD No such folder\r\n";
		}
		else
		{
			m_mess_repo.deleteFolder(folder_opt->id.value());
			response = cmd.m_tag + " OK Delete completed\r\n";
		}
	}

	m_logger.Log(TRACE, "ImapSession::HandleDelete - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleDelete - End");
	return response;
}

std::string ImapSession::HandleRename(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleRename - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleRename - Start");

	std::string response = "";
	if (cmd.m_args.size() != 2)
	{
		response = cmd.m_tag + " BAD Invalid arguments number\r\n";
	}
	else
	{
		auto folder = m_mess_repo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[0]);
		if (!folder.has_value())
		{
			response = cmd.m_tag + " NO No mailbox with specified name\r\n";
		}
		else
		{
			m_mess_repo.renameFolder(folder->id.value(), cmd.m_args[1]);
			response = cmd.m_tag + " OK Rename completed\r\n";
		}
	}

	m_logger.Log(TRACE, "ImapSession::HandleRename - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleRename - End");
	return response;
}

std::string ImapSession::HandleCopy(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleCopy - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleCopy - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = cmd.m_tag + " BAD No mailbox selected\r\n";
	}

	if (cmd.m_args.size() != 2)
	{
		response = cmd.m_tag + " BAD Missing arguments\r\n";
	}
	else
	{
		auto folder_dest_opt = m_mess_repo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[1]);
		if (!folder_dest_opt.has_value())
		{
			response = cmd.m_tag + " BAD No such folder\r\n";
		}
		else
		{
			try
			{
				auto messages = m_mess_repo.findByFolder(m_currentMailbox.m_id.value());
				IMAP_UTILS::SortMessagesByTimeDescending(messages);
				auto lists_ids = IMAP_UTILS::ParseSequenceSet(cmd.m_args[0], messages.size());

				std::vector<Message> selected_messages;
				selected_messages.reserve(lists_ids.size());

				for (int seq_num : lists_ids)
				{
					if (seq_num > 0 && seq_num <= static_cast<int>(messages.size()))
					{
						// seq_num is coming from IMAP so it is 1-based, while vector is 0-based,
						// so we need to subtract 1
						selected_messages.push_back(messages[seq_num - 1]);
					}
				}

				for (const auto& msg : selected_messages)
				{
					// FIX: we should copy message to another folder,
					// but repo only provides mechanism of moving message
					m_mess_repo.moveToFolder(msg.id.value(), folder_dest_opt->id);
				}

				response = cmd.m_tag + " OK Copy completed\r\n";
			}
			catch (const std::exception& ex)
			{
				response = cmd.m_tag + " BAD Invalid message sequence\r\n";
				m_logger.Log(PROD,
							 "ImapSession::HandleCopy - Invalid COPY usage, exception: " + std::string(ex.what()));
				m_logger.Log(TRACE, "ImapSession::HandleCopy - Out: " + response);
				m_logger.Log(DEBUG, "ImapSession::HandleCopy - End");
				return response;
			}
		}
	}

	m_logger.Log(TRACE, "ImapSession::HandleCopy - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleCopy - End");
	return response;
}

std::string ImapSession::HandleExpunge(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapSession::HandleExpunge - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapSession::HandleExpunge - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = cmd.m_tag + " BAD No mailbox selected\r\n";
	}
	else if (cmd.m_args.size() != 0)
	{
		response = cmd.m_tag + " BAD Invalid arguments\r\n";
	}
	else
	{
		auto messages = m_mess_repo.findByFolder(m_currentMailbox.m_id.value());
		IMAP_UTILS::SortMessagesByTimeDescending(messages);

		std::vector<std::pair<Message, int>> messages_indexes;
		int index = 1;
		std::transform(messages.begin(), messages.end(), std::back_inserter(messages_indexes),
					   [&index](const Message& msg) { return std::make_pair(msg, index++); });

		std::vector<std::pair<Message, int>> expunged_seq_nums;
		std::copy_if(messages_indexes.begin(), messages_indexes.end(), std::back_inserter(expunged_seq_nums),
					 [](const std::pair<Message, int>& msg_pair)
					 { return msg_pair.first.status == MessageStatus::Deleted; });

		for (const auto& [mess, index] : expunged_seq_nums)
		{
			m_mess_repo.hardDelete(mess.id.value());
			response += ImapResponse::Untagged(std::to_string(index) + " EXPUNGE\r\n");
		}

		response = cmd.m_tag + " OK Expunge completed\r\n";
	}

	m_logger.Log(TRACE, "ImapSession::HandleExpunge - Out: " + response);
	m_logger.Log(DEBUG, "ImapSession::HandleExpunge - End");
	return response;
}
