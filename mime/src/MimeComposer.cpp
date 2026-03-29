#include "MimeComposer.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace SmtpClient {

Email MimeComposer::CreateNew(const std::string& from, const std::string& to, const std::string& subject,
							  const std::string& body_text)
{
	Email email;
	email.sender     = from;
	email.to = { to };
	email.subject    = subject;
	email.plain_text = body_text;
	return email;
}

Email MimeComposer::CreateReply(const Email& original, const std::string& from,
								const std::string& reply_text)
{
	Email email;
	email.sender = from;
	email.to     = { original.sender };

	if (StartsWithIgnoreCase(original.subject, "re:"))
		email.subject = original.subject;
	else
		email.subject = "Re: " + original.subject;

	email.in_reply_to = original.message_id;
	email.references  = original.references.empty()
					   ? original.message_id
					   : original.references + " " + original.message_id;

	email.plain_text = reply_text + "\n\n" + BuildQuotedText(original);
	return email;
}

Email MimeComposer::CreateForward(const Email& original, const std::string& from,
								  const std::string& to, const std::string& comment_text)
{
	Email email;
	email.sender = from;
	email.to = { to };

	if (StartsWithIgnoreCase(original.subject, "fwd:") ||
		StartsWithIgnoreCase(original.subject, "fw:"))
		email.subject = original.subject;
	else
		email.subject = "Fwd: " + original.subject;

	email.plain_text = comment_text + "\n\n" + BuildForwardHeader(original)
					 + original.plain_text;

	email.attachments = original.attachments;
	return email;
}

bool MimeComposer::StartsWithIgnoreCase(const std::string& str, const std::string& prefix)
{
	if (str.size() < prefix.size()) return false;
	return std::equal(prefix.begin(), prefix.end(), str.begin(),
					  [](unsigned char a, unsigned char b)
					  { return std::tolower(a) == std::tolower(b); });
}

std::string MimeComposer::BuildQuotedText(const Email& original)
{
	std::ostringstream oss;
	oss << "On " << original.date << ", " << original.sender << " wrote:\n";

	std::istringstream body_stream(original.plain_text);
	std::string        line;
	while (std::getline(body_stream, line))
		oss << "> " << line << "\n";

	return oss.str();
}

std::string MimeComposer::BuildForwardHeader(const Email& original)
{
	std::ostringstream oss;
	oss << "---------- Forwarded message ----------\n";
	oss << "From: " << original.sender << "\n";
	std::string to_str;
	for (std::size_t i = 0; i < original.to.size(); ++i)
	{
		if (i > 0) to_str += ", ";
		to_str += original.to[i];
	}
	oss << "To: " << to_str << "\n";
	oss << "Date: " << original.date << "\n";
	oss << "Subject: " << original.subject << "\n";
	oss << "---------------------------------------\n\n";
	return oss.str();
}

} // namespace SmtpClient
