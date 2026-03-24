#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace SmtpClient {

/**
 * MimePart is a node in the recursive MIME tree structure of an email.
 *
 * For leaf parts (text/plain, text/html, application/pdf, etc.) children is empty.
 * For multipart nodes (multipart/mixed, multipart/alternative, multipart/related, etc.)
 * children contains the sub-parts in order, and boundary is set.
 *
 * Typical structure of an email with HTML and one attachment:
 *
 *   MimePart { type="multipart", subtype="mixed", boundary="B1" }
 *   ├── MimePart { type="multipart", subtype="alternative", boundary="B2" }
 *   │   ├── MimePart { type="text", subtype="plain", encoding="8bit" }
 *   │   └── MimePart { type="text", subtype="html",  encoding="base64" }
 *   └── MimePart { type="application", subtype="pdf", filename="report.pdf" }
 */
struct MimePart
{
	std::string type;      // "text", "multipart", "application", "image", ...
	std::string subtype;   // "plain", "html", "mixed", "alternative", "pdf", ...

	// — text/* and attachment parts —
	std::string charset;   // e.g. "utf-8"  (non-empty for text/* parts)
	std::string encoding;  // "7bit", "8bit", "base64", "quoted-printable"
	std::string filename;  // non-empty for attachment parts
	std::size_t size  = 0; // byte size of the decoded body
	std::size_t lines = 0; // line count (required by IMAP for text/* parts)

	// — multipart/* parts —
	std::string            boundary; // non-empty for multipart/* nodes
	std::vector<MimePart>  children; // sub-parts in order

	bool IsMultipart()   const { return type == "multipart"; }
	bool IsText()        const { return type == "text"; }
	bool IsAttachment()  const { return !filename.empty(); }
};

} // namespace SmtpClient
