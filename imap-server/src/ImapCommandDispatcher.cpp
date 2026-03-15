#include "ImapCommandDispatcher.hpp"

#include <algorithm>
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
			m_currentMailbox.m_recent = std::count_if(all_messages.begin(), all_messages.end(),
													  [](const Message& msg) { return !msg.is_seen; });
			m_currentMailbox.m_id = folder_opt->id;

			response = ImapResponse::Flags() + ImapResponse::Exists(m_currentMailbox.m_exists) +
					   ImapResponse::Recent(m_currentMailbox.m_recent);
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
		for (const auto& folder : folders)
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
					// recent is not implemented yet, so we return the count of unseen messages for now
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

				std::string fetch_response = "(";
				for (const auto& item : expanded_items)
				{
					if (item == "FLAGS")
					{
						fetch_response += "FLAGS (";
						if (msg.is_seen) fetch_response += "\\Seen ";
						if (msg.is_flagged) fetch_response += "\\Flagged ";

						if (fetch_response.back() == ' ' && fetch_response.substr(fetch_response.length() - 2) != " (")
						{
							fetch_response.pop_back(); // removing trailing space
						}
						fetch_response += ") ";
					}
					else if (item == "INTERNALDATE") // db uses ISO format
					{
						fetch_response += "INTERNALDATE \"" + msg.internal_date + "\" ";
					}
					else if (item == "RFC822.SIZE")
					{
						fetch_response += "RFC822.SIZE " + std::to_string(msg.size_bytes) + " ";
					}
					else if (item == "ENVELOPE")
					{
						// Format: ((name route mailbox host))
						auto format_address = [](const std::string& email) -> std::string
						{
							if (email.empty() || email == "NIL") return "NIL";
							auto at_pos = email.find('@');
							std::string mailbox = (at_pos != std::string::npos) ? email.substr(0, at_pos) : email;
							std::string host = (at_pos != std::string::npos) ? email.substr(at_pos + 1) : "unknown";
							return "((\"\" NIL \"" + mailbox + "\" \"" + host + "\"))";
						};

						// asuming sender is owner of message
						auto sender_user = m_userRepo.findByID(msg.user_id);
						std::string sender_email =
							sender_user ? (sender_user->username + "@test.com") : "unknown@test.com";

						auto all_receipients = m_messRepo.findRecipientsByMessage(msg.id.value());
						auto rec_to = std::find_if(all_receipients.begin(), all_receipients.end(),
												   [](Recipient rec) { return rec.type == RecipientType::To; });
						auto rec_cc = std::find_if(all_receipients.begin(), all_receipients.end(),
												   [](Recipient rec) { return rec.type == RecipientType::Cc; });
						auto rec_bcc = std::find_if(all_receipients.begin(), all_receipients.end(),
													[](Recipient rec) { return rec.type == RecipientType::Bcc; });

						std::string env = "(";
						env += "\"" + msg.internal_date + "\" ";   // 1. Date
						env += "\"" + msg.subject.value() + "\" "; // 2. Subject
						env += format_address(sender_email) + " "; // 3. From
						env += format_address(sender_email) + " "; // 4. Sender
						env += format_address(sender_email) + " "; // 5. Reply-To
						env +=
							(rec_to != all_receipients.end()) ? (format_address(rec_to->address) + " ") : ""; // 6. To
						env +=
							(rec_cc != all_receipients.end()) ? (format_address(rec_cc->address) + " ") : ""; // 7. Cc
						env += (rec_bcc != all_receipients.end()) ? (format_address(rec_bcc->address) + " ")
																  : "";						   // 8. Bcc
						env += msg.in_reply_to.has_value() ? msg.in_reply_to.value() : "NIL "; // 9. In-Reply-To
						env += "\"<" + std::to_string(msg.id.value()) + "@test.com>\"";		   // 10. Message-ID
						env += ")";

						fetch_response += "ENVELOPE " + env + " ";
					}
					else if (item == "BODY" || item == "RFC822" || item == "RFC822.TEXT")
					{
						fetch_response += item + " {" + std::to_string(msg.size_bytes) + "}\r\n" + "" /*msg.body*/;
					}
					else if (item == "RFC822.HEADER")
					{
						auto all_receipients = m_messRepo.findRecipientsByMessage(msg.id.value());
						auto rec_to = std::find_if(all_receipients.begin(), all_receipients.end(),
												   [](Recipient rec) { return rec.type == RecipientType::To; });
						auto sender_user = m_userRepo.findByID(msg.user_id);
						std::string sender = sender_user ? sender_user->username : "unknown";
						std::string header = "From: " + sender +
											 "@test.com\r\n"
											 "To: " +
											 ((rec_to != all_receipients.end()) ? ((rec_to->address) + " ") : "") +
											 "\r\n"
											 "Subject: " +
											 (msg.subject.has_value() ? msg.subject.value() : "") +
											 "\r\n"
											 "Date: " +
											 msg.internal_date +
											 "\r\n"
											 "\r\n"; // empty line declares end of header

						fetch_response += "RFC822.HEADER {" + std::to_string(header.size()) + "}\r\n" + header;
					}
					else if (item == "BODYSTRUCTURE")
					{
						// base structure with only one text/plain part, without attachments or nested multiparts,
						// as our schema doesn`t support them
						fetch_response +=
							"BODYSTRUCTURE (\"text\" \"plain\" (\"charset\" \"utf-8\") NIL NIL \"7bit\" " +
							std::to_string(msg.size_bytes) + " 0) ";
					}
					// MIME parts are not implemented/integrated yet
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

			auto flags = IMAP_UTILS::SplitArgs(IMAP_UTILS::TrimParentheses(cmd.m_args[2]));
			bool silence = cmd.m_args[1].find(".SILENT") != std::string::npos;
			if (cmd.m_args[1][0] == '+' || cmd.m_args[1][0] == '-')
			{
				for (size_t i = 0; i < selected_messages.size(); ++i)
				{
					for (const auto& flag : flags)
					{
						if (flag == "\\Seen")
						{
							m_messRepo.markSeen(selected_messages[i].id.value(), cmd.m_args[1][0] == '+');
							selected_messages[i].is_seen = (cmd.m_args[1][0] == '+');
						}
						else if (flag == "\\Flagged")
						{
							m_messRepo.markFlagged(selected_messages[i].id.value(), cmd.m_args[1][0] == '+');
							selected_messages[i].is_flagged = (cmd.m_args[1][0] == '+');
						}
					}
					if (!silence)
					{
						std::string flagsStr = "(FLAGS (";
						if (selected_messages[i].is_seen) flagsStr += "\\Seen ";
						if (selected_messages[i].is_flagged) flagsStr += "\\Flagged ";

						if (flagsStr.back() == ' ' && flagsStr.substr(flagsStr.length() - 2) != " (")
						{
							flagsStr.pop_back();
						}
						flagsStr += "))";
						response += ImapResponse::Fetch(lists_ids[i], flagsStr);
					}
				}
			}
			else
			{
				// repo doesn`t support setting flags directly, so we ignore this case for now
			}

			response += ImapResponse::Ok(cmd.m_tag, "Store completed");
		}
		catch (const std::exception& ex)
		{
			response = ImapResponse::Bad(cmd.m_tag, "Invalid message sequence");
			m_logger.Log(PROD, "ImapCommandDispatcher::HandleStore - Invalid STORE usage, exception: " +
								   std::string(ex.what()));
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
		response = ImapResponse::Bad(cmd.m_tag, "Invalid parametrs number");
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
			response = ImapResponse::Bad(cmd.m_tag, "Create failed: " + m_messRepo.getLastError());
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
					// FIX: we should copy message to another folder,
					// but repo only provides mechanism of moving message
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