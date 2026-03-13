#include "Recipient.h"

std::string Recipient::typeToString(RecipientType type)
{
    switch (type)
    {
        case RecipientType::To: return "to";
        case RecipientType::Cc: return "cc";
        case RecipientType::Bcc: return "bcc";
        case RecipientType::ReplyTo: return "reply-to";
    }
    return "to";
}

RecipientType Recipient::typeFromString(const std::string& str)
{
    if (str == "to") return RecipientType::To;
    if (str == "cc") return RecipientType::Cc;
    if (str == "bcc") return RecipientType::Bcc;
    if (str == "reply-to") return RecipientType::ReplyTo;
    return RecipientType::To;
}