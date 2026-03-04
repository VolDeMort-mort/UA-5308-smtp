#include "Message.h"

std::string Message::statusToString(MessageStatus status)
{
    switch(status)
    {
        case MessageStatus::Draft:    return "draft";
        case MessageStatus::Sent:     return "sent";
        case MessageStatus::Received: return "received";
        case MessageStatus::Deleted:  return "deleted";
        case MessageStatus::Failed:   return "failed";
    }

    //Log(Warning, "Unknown message status");
    return "draft";
}

MessageStatus Message::statusFromString(const std::string& str)
{
    if (str == "draft")    return MessageStatus::Draft;
    if (str == "sent")     return MessageStatus::Sent;
    if (str == "received") return MessageStatus::Received;
    if (str == "deleted")  return MessageStatus::Deleted;
    if (str == "failed")   return MessageStatus::Failed;

    //Log(Warning, "Unknown message status string");
    return MessageStatus::Draft;
}