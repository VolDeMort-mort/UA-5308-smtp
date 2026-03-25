#include "ImapCommandDispatcher.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "Entity/Folder.h"
#include "Entity/User.h"
#include "ImapParser.hpp"
#include "ImapResponse.hpp"
#include "ImapUtils.hpp"

ImapCommandDispatcher::ImapCommandDispatcher(ILogger& logger, UserRepository& userRepo, MessageRepository& messRepo)
	: m_logger(logger), m_userRepo(userRepo), m_messRepo(messRepo)
{
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
		{ImapCommandType::UidFetch, [this](const ImapCommand& cmd) { return HandleUidFetch(cmd); }},
		{ImapCommandType::UidStore, [this](const ImapCommand& cmd) { return HandleUidStore(cmd); }},
		{ImapCommandType::UidCopy, [this](const ImapCommand& cmd) { return HandleUidCopy(cmd); }},
		{ImapCommandType::Subscribe, [this](const ImapCommand& cmd) { return HandleSubscribe(cmd); }},
		{ImapCommandType::Unsubscribe, [this](const ImapCommand& cmd) { return HandleUnsubscribe(cmd); }},
		{ImapCommandType::Close, [this](const ImapCommand& cmd) { return HandleClose(cmd); }},
		{ImapCommandType::Check, [this](const ImapCommand& cmd) { return HandleCheck(cmd); }},
	};
}

std::string ImapCommandDispatcher::Dispatch(const ImapCommand& cmd)
{
	auto it = m_handlers.find(cmd.m_type);
	if (it != m_handlers.end())
	{
		return it->second(cmd);
	}
	return ImapResponse::Bad(cmd.m_tag, "Command not implemented");
}

bool ImapCommandDispatcher::RequiresAuth(ImapCommandType type) const
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::RequiresAuth - In: type=" + IMAP_UTILS::CommandTypeToString(type));
	m_logger.Log(DEBUG, "ImapCommandDispatcher::RequiresAuth - Start");

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

	m_logger.Log(TRACE, "ImapCommandDispatcher::RequiresAuth - Out: " + std::string(result ? "true" : "false"));
	m_logger.Log(DEBUG, "ImapCommandDispatcher::RequiresAuth - End");
	return result;
}

std::string ImapCommandDispatcher::HandleLogin(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleLogin - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleLogin - Start");

	std::string response;
	if (cmd.m_args.size() != 2)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Missing arguments");
		return response;
	}
	else if (m_userRepo.authorize(cmd.m_args[0], cmd.m_args[1]))
	{
		auto user_opt = m_userRepo.findByUsername(cmd.m_args[0]);
		if (user_opt.has_value())
		{
			m_authenticatedUserName = cmd.m_args[0];
			m_authenticatedUserID = user_opt->id;
			m_state = SessionState::Authenticated;

			m_logger.Log(PROD, "ImapCommandDispatcher::HandleLogin - Authenticated user ID: " +
								   std::to_string(m_authenticatedUserID.value()));
			m_logger.Log(PROD, "ImapCommandDispatcher::HandleLogin - User authenticated: " + cmd.m_args[0]);
			response = ImapResponse::Ok(cmd.m_tag, "Login successful");
		}
		else
		{
			m_logger.Log(
				PROD, "ImapCommandDispatcher::HandleLogin - Database inconsistency: Authenticated user ID not found");
			response = ImapResponse::No(cmd.m_tag, "Login failed");
		}
	}
	else
	{
		response = ImapResponse::No(cmd.m_tag, "Login failed");
		m_logger.Log(PROD, "ImapCommandDispatcher::HandleLogin - Login failed, error: " + m_userRepo.getLastError());
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleLogin - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleLogin - End");
	return response;
}

std::string ImapCommandDispatcher::HandleLogout(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleLogout - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleLogout - Start");

	std::string response = ImapResponse::Untagged("BYE Logout") + ImapResponse::Ok(cmd.m_tag, "Logout completed");

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleLogout - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleLogout - End");
	return response;
}

std::string ImapCommandDispatcher::HandleCapability(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleCapability - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleCapability - Start");

	std::string response = ImapResponse::Capability() + ImapResponse::Ok(cmd.m_tag, "Capability completed");

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleCapability - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleCapability - End");
	return response;
}

std::string ImapCommandDispatcher::HandleNoop(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleNoop - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleNoop - Start");

	std::string response = ImapResponse::Ok(cmd.m_tag, "Noop completed");

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleNoop - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleNoop - End");
	return response;
}

std::string ImapCommandDispatcher::HandleSelect(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleSelect - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleSelect - Start");

	std::string response;
	if (cmd.m_args.size() != 1)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Invalid arguments number");
	}
	else
	{
		auto folder_opt = m_messRepo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[0]);
		if (folder_opt.has_value())
		{
			m_currentMailbox.m_name = folder_opt->name;
			auto all_messages = m_messRepo.findByFolder(folder_opt->id.value());
			m_currentMailbox.m_exists = all_messages.size();
			// TODO: Call m_messRepo.clearRecentByFolder(folder_opt->id.value()) before fetching messages
			m_currentMailbox.m_recent = std::count_if(all_messages.begin(), all_messages.end(),
													  [](const Message& msg) { return msg.is_recent; });
			m_currentMailbox.m_id = folder_opt->id;

			size_t unseen_count = std::count_if(all_messages.begin(), all_messages.end(),
												[](const Message& msg) { return !msg.is_seen; });
			int64_t uidnext = folder_opt->next_uid;
			int64_t uidvalidity = folder_opt->id.value();

			response = ImapResponse::Flags();
			response += "* OK [UIDVALIDITY " + std::to_string(uidvalidity) + "]\r\n";
			response += "* OK [PERMANENTFLAGS (\\Seen \\Answered \\Flagged \\Draft \\Deleted \\*)]\r\n";
			// should return first UID of unseen message not count
			response += "* OK [UNSEEN " + std::to_string(unseen_count) + "]\r\n";
			response += ImapResponse::Exists(m_currentMailbox.m_exists);
			response += "* OK [UIDNEXT " + std::to_string(uidnext) + "]\r\n";
			response += ImapResponse::Recent(m_currentMailbox.m_recent);
			response += ImapResponse::Ok(cmd.m_tag, "[READ-WRITE] Select completed");
			m_state = SessionState::Selected;
			m_logger.Log(PROD, "ImapCommandDispatcher::HandleSelect - Mailbox selected: " + m_currentMailbox.m_name);
		}
		else
		{
			response = ImapResponse::Bad(cmd.m_tag, "Mailbox not found");
			m_logger.Log(PROD, "ImapCommandDispatcher::HandleSelect - Mailbox not found: " + cmd.m_args[0]);
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleSelect - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleSelect - End");
	return response;
}

std::string ImapCommandDispatcher::HandleList(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleList - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleList - Start");

	std::string response = "";

	if (cmd.m_args.size() != 2)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Invalid arguments number");
	}
	else if (cmd.m_args[1] == "")
	{
		response = ImapResponse::List('/', "", "Noselect");
		response += ImapResponse::Ok(cmd.m_tag, "List completed");
	}
	else
	{
		auto folders = m_messRepo.findFoldersByUser(m_authenticatedUserID.value());
		for (const auto& folder : folders)
		{
			response += ImapResponse::List('/', folder.name, "HasNoChildren");
		}
		response += ImapResponse::Ok(cmd.m_tag, "List completed");
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleList - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleList - End");
	return response;
}

std::string ImapCommandDispatcher::HandleLsub(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleLsub - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleLsub - Start");

	std::string response = "";

	if (cmd.m_args.size() != 2)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Invalid arguments number");
	}
	else if (cmd.m_args[1] == "")
	{
		response = ImapResponse::Lsub('/', "", "Noselect");
		response += ImapResponse::Ok(cmd.m_tag, "Lsub completed");
	}
	else
	{
		auto folders = m_messRepo.findFoldersByUser(m_authenticatedUserID.value());
		std::vector<Folder> subscribed_folders;
		std::copy_if(folders.begin(), folders.end(), std::back_inserter(subscribed_folders),
					 [](Folder& folder) { return folder.is_subscribed; });
		for (const auto& folder : subscribed_folders)
		{
			response += ImapResponse::Lsub('/', folder.name, "HasNoChildren");
		}
		response += ImapResponse::Ok(cmd.m_tag, "Lsub completed");
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleLsub - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleLsub - End");
	return response;
}

std::string ImapCommandDispatcher::HandleStatus(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleStatus - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleStatus - Start");

	std::string response = "";
	if (cmd.m_args.size() != 2)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Invalid arguments number");
	}
	else
	{
		auto folder_opt = m_messRepo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[0]);
		if (folder_opt.has_value())
		{
			auto all_messages = m_messRepo.findByFolder(folder_opt->id.value());
			auto reqs = IMAP_UTILS::SplitArgs(IMAP_UTILS::TrimParentheses(cmd.m_args[1]));
			bool success = false;

			for (auto& req : reqs)
			{
				if (req == "MESSAGES")
				{
					success = true;
					response += "MESSAGES " + std::to_string(all_messages.size()) + " ";
				}
				else if (req == "RECENT")
				{
					success = true;
					size_t recent = std::count_if(all_messages.begin(), all_messages.end(),
												  [](const Message& msg) { return msg.is_recent; });
					response += "RECENT " + std::to_string(recent) + " ";
				}
				else if (req == "UIDNEXT")
				{
					success = true;
					response += "UIDNEXT " + std::to_string(folder_opt->next_uid) + " ";
				}
				else if (req == "UIDVALIDITY")
				{
					success = true;
					response += "UIDVALIDITY " + std::to_string(folder_opt->id.value()) + " ";
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
				if (!response.empty() && response.back() == ' ')
				{
					response.pop_back();
				}

				response = ImapResponse::Status(cmd.m_args[0], response);
				response += ImapResponse::Ok(cmd.m_tag, "Status completed");
			}
			else
			{
				response = ImapResponse::Bad(cmd.m_tag, "Invalid status data items");
				m_logger.Log(PROD, "ImapCommandDispatcher::HandleStatus - Invalid status data items: " + cmd.m_args[1]);
			}
		}
		else
		{
			response = ImapResponse::Bad(cmd.m_tag, "Mailbox not found");
			m_logger.Log(PROD, "ImapCommandDispatcher::HandleStatus - Mailbox not found: " + cmd.m_args[0]);
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleStatus - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleStatus - End");
	return response;
}

std::string ImapCommandDispatcher::HandleFetch(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleFetch - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleFetch - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = ImapResponse::Bad(cmd.m_tag, "No mailbox selected");
	}
	else if (cmd.m_args.size() < 2)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Missing arguments");
	}
	else
	{
		try
		{
			auto messages = m_messRepo.findByFolder(m_currentMailbox.m_id.value());
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

			// Combine split BODY[HEADER.FIELDS ...] sections
			data_items = IMAP_UTILS::CombineSplitBodySections(data_items);

			// expanding macroes
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

			for (size_t i = 0; i < selected_messages.size(); ++i)
			{
				const auto& msg = selected_messages[i];
				size_t seq_num = lists_ids[i];

				auto email_opt = IMAP_UTILS::GetParsedEmail(msg, m_logger);
				auto mime_part_otp = IMAP_UTILS::GetParsedMimePart(msg, m_logger);

				std::string fetch_response = "(";
				for (const auto& item : expanded_items)
				{
					if (item == "FLAGS")
					{
						fetch_response += "FLAGS (";
						if (msg.is_seen) fetch_response += "\\Seen ";
						if (msg.is_deleted) fetch_response += "\\Deleted ";
						if (msg.is_draft) fetch_response += "\\Draft ";
						if (msg.is_answered) fetch_response += "\\Answered ";
						if (msg.is_flagged) fetch_response += "\\Flagged ";
						if (msg.is_recent) fetch_response += "\\Recent ";
						if (fetch_response.back() == ' ') fetch_response.pop_back();
						fetch_response += ") ";
					}
					else if (item == "INTERNALDATE")
					{
						fetch_response += "INTERNALDATE \"" + IMAP_UTILS::DateToIMAPInternal(msg.internal_date) + "\" ";
					}
					else if (item == "RFC822.SIZE")
					{
						fetch_response += "RFC822.SIZE " + std::to_string(msg.size_bytes) + " ";
					}
					else if (item == "ENVELOPE")
					{
						fetch_response += "ENVELOPE " + IMAP_UTILS::BuildEnvelope(msg, email_opt, m_messRepo) + " ";
					}
					else if (item == "BODY[]" || item == "RFC822" || item == "RFC822.TEXT")
					{
						std::string body = IMAP_UTILS::GetBodyContent(msg);
						fetch_response += item + " {" + std::to_string(body.size()) + "}\r\n" + body + " ";
					}
					else if (item == "RFC822.HEADER")
					{
						std::string header;
						if (email_opt)
						{
							std::string to_rec;
							for (const auto& rec : email_opt->to)
							{
								if (!to_rec.empty()) to_rec += ", ";
								to_rec += rec;
							}

							std::string cc_rec;
							for (const auto& rec : email_opt->cc)
							{
								if (!cc_rec.empty()) cc_rec += ", ";
								cc_rec += rec;
							}

							header = "From: " + email_opt->sender + "\r\n" + "To: " + to_rec + "\r\n" +
									 (cc_rec == "" ? "" : ("Cc:" + cc_rec + "\r\n")) +
									 "Subject: " + email_opt->subject + "\r\n" + "Date: " + email_opt->date + "\r\n" +
									 "Message-ID: " + email_opt->message_id + "\r\n" + "\r\n";
						}
						else
						{
							auto sender_user = m_userRepo.findByID(msg.user_id);
							std::string sender = sender_user ? sender_user->username : "unknown";

							auto all_receipients = m_messRepo.findRecipientsByMessage(msg.id.value());
							std::string to_addresses;
							std::string cc_addresses;

							for (const auto& rec : all_receipients)
							{
								if (rec.type == RecipientType::To)
								{
									if (!to_addresses.empty()) to_addresses += ", ";
									to_addresses += rec.address;
								}
								else if (rec.type == RecipientType::Cc)
								{
									if (!cc_addresses.empty()) cc_addresses += ", ";
									cc_addresses += rec.address;
								}
							}

							header = "From: " + sender + "@test.com\r\n" + "To: " + to_addresses + "\r\n" +
									 (cc_addresses == "" ? "" : ("Cc: " + cc_addresses + "\r\n")) +
									 "Subject: " + (msg.subject.has_value() ? msg.subject.value() : "") + "\r\n" +
									 "Date: " + msg.internal_date + "\r\n" + "\r\n";
						}
						fetch_response += "RFC822.HEADER {" + std::to_string(header.size()) + "}\r\n" + header;
					}
					else if (item == "BODYSTRUCTURE")
					{
						fetch_response +=
							"BODYSTRUCTURE " + IMAP_UTILS::BuildBodystructure(msg, email_opt, mime_part_otp) + " ";
					}
					else if (item == "BODY")
					{
						fetch_response += "BODY " + IMAP_UTILS::BuildBodystructure(msg, email_opt, mime_part_otp) + " ";
					}
					else if (item == "BODY[HEADER.FIELDS]" || item == "BODY.PEEK[HEADER.FIELDS]")
					{
						std::string raw_mime = IMAP_UTILS::GetBodyContent(msg);
						if (raw_mime.empty()) return "";

						std::string headers, body;
						SmtpClient::MimeParser::SplitHeadersAndBody(raw_mime, headers, body);
						std::string headers_list = IMAP_UTILS::TrimParentheses(cmd.m_args[2]);

						std::vector<std::string> requested_headers;
						std::istringstream iss(headers_list);
						std::string h;
						while (iss >> h)
						{
							requested_headers.push_back(h);
						}

						std::string result;
						for (const auto& req : requested_headers)
						{
							std::string value = SmtpClient::MimeParser::GetHeaderValue(headers, req + ":");
							if (!value.empty())
							{
								result += req + ": " + value + "\r\n";
							}
						}

						result += "\r\n";

						std::string item_name = item;
						if (item_name.back() == ']')
						{
							item_name.pop_back();
							item_name += " (" + headers_list + ")]";
						}
						else
						{
							item_name += " (" + headers_list + ")";
						}

						fetch_response += item_name + " {" + std::to_string(result.size()) + "}\r\n" + result + " ";
					}
					else if (item.rfind("BODY[", 0) == 0 || item.rfind("BODY.PEEK[", 0) == 0)
					{
						bool is_peek = (item.rfind("BODY.PEEK[", 0) == 0);
						// 10 is the index of number in BODY.PEEK[*..., 5 - BODY[*..
						size_t bracket_start = is_peek ? 10 : 5;
						size_t bracket_end = item.find(']', bracket_start);
						if (bracket_end == std::string::npos)
						{
							throw std::invalid_argument("Invalid BODY section: " + item);
						}
						std::string section = item.substr(bracket_start, bracket_end - bracket_start);

						std::string body_content = IMAP_UTILS::GetBodySection(msg, section);
						std::string item_name = is_peek ? "BODY.PEEK[" + section + "]" : "BODY[" + section + "]";
						fetch_response +=
							item_name + " {" + std::to_string(body_content.size()) + "}\r\n" + body_content + " ";
					}
					else if (item == "UID")
					{
						fetch_response += "UID " + std::to_string(msg.uid) + " ";
					}
					else
					{
						throw std::invalid_argument("Invalid fetch attribute: " + item);
					}
				}

				if (!fetch_response.empty() && fetch_response.back() == ' ')
				{
					fetch_response.pop_back(); // removing trailing space
				}

				fetch_response += ")";
				response += ImapResponse::Fetch(seq_num, fetch_response);
			}

			response += ImapResponse::Ok(cmd.m_tag, "Fetch completed");
		}
		catch (const std::invalid_argument& ex)
		{
			response = ImapResponse::Bad(cmd.m_tag, ex.what());
		}
		catch (const std::exception& ex)
		{
			response = ImapResponse::Bad(cmd.m_tag, "Invalid message sequence");
			m_logger.Log(PROD, "ImapCommandDispatcher::handleFetch - Invalid FETCH usage, exception: " +
								   std::string(ex.what()));
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleFetch - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleFetch - End");
	return response;
}

std::string ImapCommandDispatcher::HandleStore(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleStore - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleStore - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = ImapResponse::Bad(cmd.m_tag, "No mailbox selected");
	}
	else if (cmd.m_args.size() != 3)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Invalid arguments number");
	}
	else
	{
		try
		{
			// db doesn`t support custom flags
			auto raw_flags = IMAP_UTILS::SplitArgs(IMAP_UTILS::TrimParentheses(cmd.m_args[2]));
			const std::set<std::string> valid_system_flags = {"\\Seen",		"\\Deleted", "\\Draft",
															  "\\Answered", "\\Flagged", "\\Recent"};

			for (const auto& f : raw_flags)
			{
				if (valid_system_flags.find(f) == valid_system_flags.end())
				{
					return ImapResponse::Bad(cmd.m_tag, "Unknown flag: " + f);
				}
			}

			char operation = cmd.m_args[1][0]; // '+', '-' or 'F'
			bool is_silence = cmd.m_args[1].find(".SILENT") != std::string::npos;

			auto all_messages = m_messRepo.findByFolder(m_currentMailbox.m_id.value());
			auto seq_ids = IMAP_UTILS::ParseSequenceSet(cmd.m_args[0], all_messages.size());

			std::string fetch_responses = "";

			for (int seq_num : seq_ids)
			{
				if (seq_num < 1 || seq_num > all_messages.size())
				{
					continue;
				}

				auto& msg = all_messages[seq_num - 1];

				// repository format
				std::vector<std::string> flags_to_send = raw_flags;
				if (operation == '+' || operation == '-')
				{
					for (auto& f : flags_to_send)
						f = operation + f;
				}

				if (m_messRepo.setFlags(msg.id.value(), flags_to_send))
				{
					if (!is_silence)
					{
						auto updated = m_messRepo.findByID(msg.id.value());
						if (updated)
						{
							fetch_responses += ImapResponse::Fetch(seq_num, IMAP_UTILS::FormatFlagsResponse(*updated));
						}
					}
				}
			}

			response = fetch_responses + ImapResponse::Ok(cmd.m_tag, "Store completed");
		}
		catch (const std::exception& ex)
		{
			response = ImapResponse::Bad(cmd.m_tag, "Error processing STORE");
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleStore - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleStore - End");
	return response;
}

std::string ImapCommandDispatcher::HandleCreate(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleCreate - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleCreate - Start");

	std::string response = "";
	if (cmd.m_args.size() != 1)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Invalid parameters number");
	}
	else
	{
		// with name like parent/child doesn`t handle well, because we don`t support hierarchy in schema,
		// so we just create folder with such name without parent-child relation

		Folder f{std::nullopt, m_authenticatedUserID.value(), cmd.m_args[0]};
		if (m_messRepo.createFolder(f))
		{
			response = ImapResponse::Ok(cmd.m_tag, "Create completed");
		}
		else
		{
			m_logger.Log(PROD, "HandleCreate failed: " + m_messRepo.getLastError());
			response = ImapResponse::No(cmd.m_tag, "Create failed: " + m_messRepo.getLastError());
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleCreate - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleCreate - End");
	return response;
}

std::string ImapCommandDispatcher::HandleDelete(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleDelete - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleDelete - Start");

	std::string response = "";
	if (cmd.m_args.size() != 1)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Invalid parameters number");
	}
	else
	{
		auto folder_opt = m_messRepo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[0]);
		if (!folder_opt.has_value())
		{
			response = ImapResponse::Bad(cmd.m_tag, "No such folder");
		}
		else if (folder_opt->name == "INBOX")
		{
			response = ImapResponse::No(cmd.m_tag, "Can`t delete INBOX folder");
		}
		else
		{
			if (m_messRepo.deleteFolder(folder_opt->id.value()))
			{
				response = ImapResponse::Ok(cmd.m_tag, "Delete completed");
			}
			else
			{
				m_logger.Log(PROD, "HandleDelete failed: " + m_messRepo.getLastError());
				response = ImapResponse::Bad(cmd.m_tag, "Delete failed: " + m_messRepo.getLastError());
			}
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleDelete - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleDelete - End");
	return response;
}

std::string ImapCommandDispatcher::HandleRename(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleRename - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleRename - Start");

	std::string response = "";
	if (cmd.m_args.size() != 2)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Invalid arguments number");
	}
	else
	{
		auto folder = m_messRepo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[0]);
		if (!folder.has_value())
		{
			response = ImapResponse::No(cmd.m_tag, "No mailbox with specified name");
		}
		else
		{
			if (m_messRepo.renameFolder(folder->id.value(), cmd.m_args[1]))
			{
				response = ImapResponse::Ok(cmd.m_tag, "Rename completed");
			}
			else
			{
				m_logger.Log(PROD, "HandleRename failed: " + m_messRepo.getLastError());
				response = ImapResponse::Bad(cmd.m_tag, "Rename failed: " + m_messRepo.getLastError());
			}
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleRename - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleRename - End");
	return response;
}

std::string ImapCommandDispatcher::HandleCopy(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleCopy - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleCopy - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = ImapResponse::Bad(cmd.m_tag, "No mailbox selected");
	}
	else if (cmd.m_args.size() != 2)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Missing arguments");
	}
	else
	{
		auto folder_dest_opt = m_messRepo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[1]);
		if (!folder_dest_opt.has_value())
		{
			response = ImapResponse::Bad(cmd.m_tag, "No such folder");
		}
		else
		{
			try
			{
				auto messages = m_messRepo.findByFolder(m_currentMailbox.m_id.value());
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
					m_messRepo.copy(msg.id.value(), folder_dest_opt->id.value());
				}

				response = ImapResponse::Ok(cmd.m_tag, "Copy completed");
			}
			catch (const std::exception& ex)
			{
				response = ImapResponse::Bad(cmd.m_tag, "Invalid message sequence");
				m_logger.Log(PROD, "ImapCommandDispatcher::handleCopy - Invalid COPY usage, exception: " +
									   std::string(ex.what()));
			}
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleCopy - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleCopy - End");
	return response;
}

std::string ImapCommandDispatcher::HandleExpunge(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleExpunge - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleExpunge - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = ImapResponse::Bad(cmd.m_tag, "No mailbox selected");
	}
	else if (cmd.m_args.size() != 0)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Invalid arguments");
	}
	else
	{
		auto messages = m_messRepo.findByFolder(m_currentMailbox.m_id.value());

		std::vector<std::pair<Message, int>> messages_indexes;
		int index = 1;
		std::transform(messages.begin(), messages.end(), std::back_inserter(messages_indexes),
					   [&index](const Message& msg) { return std::make_pair(msg, index++); });

		std::vector<std::pair<Message, int>> expunged_seq_nums;
		std::copy_if(messages_indexes.begin(), messages_indexes.end(), std::back_inserter(expunged_seq_nums),
					 [](const std::pair<Message, int>& msg_pair) { return msg_pair.first.is_deleted; });

		// sorting by sequence numbers to send expunged responses in correct order
		std::sort(expunged_seq_nums.begin(), expunged_seq_nums.end(),
				  [](const std::pair<Message, int>& a, const std::pair<Message, int>& b)
				  { return a.second > b.second; });

		for (const auto& [mess, index] : expunged_seq_nums)
		{
			if (m_messRepo.hardDelete(mess.id.value()))
			{
				response += ImapResponse::Untagged(std::to_string(index) + " EXPUNGE");
			}
			else
			{
				m_logger.Log(PROD, "HandleExpunge failed to delete message: " + m_messRepo.getLastError());
			}
		}

		response += ImapResponse::Ok(cmd.m_tag, "Expunge completed");
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleExpunge - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleExpunge - End");
	return response;
}

std::string ImapCommandDispatcher::HandleUidFetch(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleUidFetch - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleUidFetch - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = ImapResponse::Bad(cmd.m_tag, "No mailbox selected");
	}
	else if (cmd.m_args.size() < 2)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Missing arguments");
	}
	else
	{
		try
		{
			auto uids = IMAP_UTILS::ParseSequenceSet(cmd.m_args[0], INT64_MAX);
			auto all_messages = m_messRepo.findByFolder(m_currentMailbox.m_id.value());
			auto data_items_str = IMAP_UTILS::TrimParentheses(cmd.m_args[1]);
			auto data_items = IMAP_UTILS::SplitArgs(data_items_str);

			// Combine split BODY[HEADER.FIELDS ...] sections
			data_items = IMAP_UTILS::CombineSplitBodySections(data_items);

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

			for (int64_t uid : uids)
			{
				auto msg_opt = m_messRepo.findByUID(m_currentMailbox.m_id.value(), uid);
				if (!msg_opt.has_value()) continue;

				const auto& msg = msg_opt.value();

				auto email_opt = IMAP_UTILS::GetParsedEmail(msg, m_logger);
				auto mime_part_otp = IMAP_UTILS::GetParsedMimePart(msg, m_logger);

				size_t seq_num = 0;
				for (size_t i = 0; i < all_messages.size(); ++i)
				{
					if (all_messages[i].uid == uid)
					{
						seq_num = i + 1; // sequence numbers are 1-based
						break;
					}
				}

				std::string fetch_response = "(UID " + std::to_string(msg.uid) + " ";
				for (const auto& item : expanded_items)
				{
					if (item == "FLAGS")
					{
						fetch_response += "FLAGS (";
						if (msg.is_seen) fetch_response += "\\Seen ";
						if (msg.is_deleted) fetch_response += "\\Deleted ";
						if (msg.is_draft) fetch_response += "\\Draft ";
						if (msg.is_answered) fetch_response += "\\Answered ";
						if (msg.is_flagged) fetch_response += "\\Flagged ";
						if (msg.is_recent) fetch_response += "\\Recent ";
						if (fetch_response.back() == ' ') fetch_response.pop_back();
						fetch_response += ") ";
					}
					else if (item == "INTERNALDATE")
					{
						fetch_response += "INTERNALDATE \"" + IMAP_UTILS::DateToIMAPInternal(msg.internal_date) + "\" ";
					}
					else if (item == "RFC822.SIZE")
					{
						fetch_response += "RFC822.SIZE " + std::to_string(msg.size_bytes) + " ";
					}
					else if (item == "ENVELOPE")
					{
						fetch_response += "ENVELOPE " + IMAP_UTILS::BuildEnvelope(msg, email_opt, m_messRepo) + " ";
					}
					else if (item == "BODY[]" || item == "RFC822" || item == "RFC822.TEXT")
					{
						std::string body = IMAP_UTILS::GetBodyContent(msg);
						fetch_response += item + " {" + std::to_string(body.size()) + "}\r\n" + body + " ";
					}
					else if (item == "RFC822.HEADER")
					{
						std::string header;
						if (email_opt)
						{
							std::string to_rec;
							for (const auto& rec : email_opt->to)
							{
								if (!to_rec.empty()) to_rec += ", ";
								to_rec += rec;
							}

							std::string cc_rec;
							for (const auto& rec : email_opt->cc)
							{
								if (!cc_rec.empty()) cc_rec += ", ";
								cc_rec += rec;
							}

							header = "From: " + email_opt->sender + "\r\n" + "To: " + to_rec + "\r\n" +
									 (cc_rec == "" ? "" : ("Cc:" + cc_rec + "\r\n")) +
									 "Subject: " + email_opt->subject + "\r\n" + "Date: " + email_opt->date + "\r\n" +
									 "Message-ID: " + email_opt->message_id + "\r\n" + "\r\n";
						}
						else
						{
							auto sender_user = m_userRepo.findByID(msg.user_id);
							std::string sender = sender_user ? sender_user->username : "unknown";

							auto all_receipients = m_messRepo.findRecipientsByMessage(msg.id.value());
							std::string to_addresses;
							std::string cc_addresses;

							for (const auto& rec : all_receipients)
							{
								if (rec.type == RecipientType::To)
								{
									if (!to_addresses.empty()) to_addresses += ", ";
									to_addresses += rec.address;
								}
								else if (rec.type == RecipientType::Cc)
								{
									if (!cc_addresses.empty()) cc_addresses += ", ";
									cc_addresses += rec.address;
								}
							}

							header = "From: " + sender + "@test.com\r\n" + "To: " + to_addresses + "\r\n" +
									 (cc_addresses == "" ? "" : ("Cc: " + cc_addresses + "\r\n")) +
									 "Subject: " + (msg.subject.has_value() ? msg.subject.value() : "") + "\r\n" +
									 "Date: " + msg.internal_date + "\r\n" + "\r\n";
						}
						fetch_response += "RFC822.HEADER {" + std::to_string(header.size()) + "}\r\n" + header;
					}
					else if (item == "BODYSTRUCTURE")
					{
						fetch_response +=
							"BODYSTRUCTURE " + IMAP_UTILS::BuildBodystructure(msg, email_opt, mime_part_otp) + " ";
					}
					else if (item == "BODY")
					{
						fetch_response += "BODY " + IMAP_UTILS::BuildBodystructure(msg, email_opt, mime_part_otp) + " ";
					}
					else if (item.rfind("BODY[", 0) == 0 || item.rfind("BODY.PEEK[", 0) == 0)
					{
						bool is_peek = (item.rfind("BODY.PEEK[", 0) == 0);
						size_t bracket_start = is_peek ? 10 : 5;
						size_t bracket_end = item.find(']', bracket_start);
						if (bracket_end == std::string::npos)
						{
							throw std::invalid_argument("Invalid BODY section: " + item);
						}
						std::string section = item.substr(bracket_start, bracket_end - bracket_start);

						std::string body_content = IMAP_UTILS::GetBodySection(msg, section);
						std::string item_name = is_peek ? "BODY.PEEK[" + section + "]" : "BODY[" + section + "]";
						fetch_response +=
							item_name + " {" + std::to_string(body_content.size()) + "}\r\n" + body_content + " ";
					}
					else if (item == "UID")
					{
						fetch_response += "UID " + std::to_string(msg.uid) + " ";
					}
					else
					{
						throw std::invalid_argument("Invalid fetch attribute: " + item);
					}
				}

				if (!fetch_response.empty() && fetch_response.back() == ' ')
				{
					fetch_response.pop_back();
				}

				fetch_response += ")";
				response += ImapResponse::Fetch(seq_num, fetch_response);
			}

			response += ImapResponse::Ok(cmd.m_tag, "Uid Fetch completed");
		}
		catch (const std::invalid_argument& ex)
		{
			response = ImapResponse::Bad(cmd.m_tag, ex.what());
		}
		catch (const std::exception& ex)
		{
			response = ImapResponse::Bad(cmd.m_tag, "Invalid message sequence");
			m_logger.Log(PROD, "ImapCommandDispatcher::HandleUidFetch - Invalid UID FETCH usage, exception: " +
								   std::string(ex.what()));
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleUidFetch - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleUidFetch - End");
	return response;
}

std::string ImapCommandDispatcher::HandleUidStore(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleUidStore - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleUidStore - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = ImapResponse::Bad(cmd.m_tag, "No mailbox selected");
	}
	else if (cmd.m_args.size() != 3)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Missing arguments");
	}
	else
	{
		try
		{
			// db doesn`t support custom flags
			auto raw_flags = IMAP_UTILS::SplitArgs(IMAP_UTILS::TrimParentheses(cmd.m_args[2]));
			const std::set<std::string> valid_system_flags = {"\\Seen",		"\\Deleted", "\\Draft",
															  "\\Answered", "\\Flagged", "\\Recent"};

			bool all_flags_valid = true;
			for (const auto& f : raw_flags)
			{
				if (valid_system_flags.find(f) == valid_system_flags.end())
				{
					response = ImapResponse::Bad(cmd.m_tag, "Unknown flag: " + f);
					all_flags_valid = false;
					break;
				}
			}

			if (all_flags_valid)
			{
				char operation = cmd.m_args[1][0]; // '+', '-' or 'F'
				bool is_silence = cmd.m_args[1].find(".SILENT") != std::string::npos;

				auto uids = IMAP_UTILS::ParseSequenceSet(cmd.m_args[0], INT64_MAX);
				std::string fetch_responses = "";

				for (size_t i = 0; i < uids.size(); ++i)
				{
					int64_t uid = uids[i];
					auto msg_opt = m_messRepo.findByUID(m_currentMailbox.m_id.value(), uid);
					if (!msg_opt.has_value()) continue;

					// repository format
					std::vector<std::string> flags_to_send = raw_flags;
					if (operation == '+' || operation == '-')
					{
						for (auto& f : flags_to_send)
							f = operation + f;
					}

					if (m_messRepo.setFlags(msg_opt->id.value(), flags_to_send))
					{
						if (!is_silence)
						{
							auto updated = m_messRepo.findByID(msg_opt->id.value());
							if (updated)
							{
								// UID STORE response must include UID token
								std::string flags_body = "(UID " + std::to_string(updated->uid) + " " +
														 IMAP_UTILS::FormatFlagsResponse(*updated) + ")";
								fetch_responses += ImapResponse::Fetch(i + 1, flags_body);
							}
						}
					}
				}
				response = fetch_responses + ImapResponse::Ok(cmd.m_tag, "Uid Store completed");
			}
		}
		catch (const std::exception& ex)
		{
			response = ImapResponse::Bad(cmd.m_tag, "Invalid message sequence");
			m_logger.Log(PROD, "ImapCommandDispatcher::HandleUidStore - Invalid UID STORE usage, exception: " +
								   std::string(ex.what()));
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleUidStore - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleUidStore - End");
	return response;
}

std::string ImapCommandDispatcher::HandleUidCopy(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleUidCopy - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleUidCopy - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = ImapResponse::Bad(cmd.m_tag, "No mailbox selected");
	}
	else if (cmd.m_args.size() != 2)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Missing arguments");
	}
	else
	{
		auto folder_dest_opt = m_messRepo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[1]);
		if (!folder_dest_opt.has_value())
		{
			response = ImapResponse::Bad(cmd.m_tag, "No such folder");
		}
		else
		{
			try
			{
				auto uids = IMAP_UTILS::ParseSequenceSet(cmd.m_args[0], INT64_MAX);

				for (int64_t uid : uids)
				{
					auto msg_opt = m_messRepo.findByUID(m_currentMailbox.m_id.value(), uid);
					if (!msg_opt.has_value()) continue;

					m_messRepo.copy(msg_opt->id.value(), folder_dest_opt->id.value());
				}

				response = ImapResponse::Ok(cmd.m_tag, "Uid Copy completed");
			}
			catch (const std::exception& ex)
			{
				response = ImapResponse::Bad(cmd.m_tag, "Invalid message sequence");
				m_logger.Log(PROD, "ImapCommandDispatcher::HandleUidCopy - Invalid UID COPY usage, exception: " +
									   std::string(ex.what()));
			}
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleUidCopy - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleUidCopy - End");
	return response;
}

// currently does nothing
std::string ImapCommandDispatcher::HandleSubscribe(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleSubscribe - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleSubscribe - Start");

	std::string response = "";
	if (cmd.m_args.size() != 1)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Invalid arguments number");
	}
	else
	{
		auto folder_opt = m_messRepo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[0]);
		if (!folder_opt.has_value())
		{
			response = ImapResponse::Bad(cmd.m_tag, "Mailbox does not exist");
		}
		else
		{
			// repo doesn`t support this yet
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleSubscribe - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleSubscribe - End");
	return response;
}

// currently does nothing
std::string ImapCommandDispatcher::HandleUnsubscribe(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleUnsubscribe - In: tag=" + cmd.m_tag + ", args=[" +
							IMAP_UTILS::JoinArgs(cmd.m_args) + "]");
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleUnsubscribe - Start");

	std::string response = "";
	if (cmd.m_args.size() != 1)
	{
		response = ImapResponse::Bad(cmd.m_tag, "Invalid arguments number");
	}
	else
	{
		auto folder_opt = m_messRepo.findFolderByName(m_authenticatedUserID.value(), cmd.m_args[0]);
		if (!folder_opt.has_value())
		{
			response = ImapResponse::Bad(cmd.m_tag, "Mailbox does not exist");
		}
		else
		{
			// repo doesn`t support this yet
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleUnsubscribe - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleUnsubscribe - End");
	return response;
}

std::string ImapCommandDispatcher::HandleClose(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleClose - In: tag=" + cmd.m_tag);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleClose - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = ImapResponse::Bad(cmd.m_tag, "No mailbox selected");
	}
	else
	{
		if (m_messRepo.expunge(m_currentMailbox.m_id.value()))
		{
			response = ImapResponse::Ok(cmd.m_tag, "Close completed");
			m_state = SessionState::Authenticated;
			m_currentMailbox = MailboxState{};
		}
		else
		{
			response = ImapResponse::No(cmd.m_tag, "Close failed");
			m_logger.Log(DEBUG, "Close failed: " + m_messRepo.getLastError());
		}
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleClose - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleClose - End");
	return response;
}

std::string ImapCommandDispatcher::HandleCheck(const ImapCommand& cmd)
{
	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleCheck - In: tag=" + cmd.m_tag);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleCheck - Start");

	std::string response = "";
	if (m_state != SessionState::Selected)
	{
		response = ImapResponse::Bad(cmd.m_tag, "No mailbox selected");
	}
	else
	{
		response = ImapResponse::Ok(cmd.m_tag, "Check completed");
	}

	m_logger.Log(TRACE, "ImapCommandDispatcher::HandleCheck - Out: " + response);
	m_logger.Log(DEBUG, "ImapCommandDispatcher::HandleCheck - End");
	return response;
}