#include <gtest/gtest.h>

#include "Entity/Recipient.h"

TEST(RecipientTypeTest, TypeToStringTo) {
    EXPECT_EQ(Recipient::typeToString(RecipientType::To), "to");
}

TEST(RecipientTypeTest, TypeToStringCc) {
    EXPECT_EQ(Recipient::typeToString(RecipientType::Cc), "cc");
}

TEST(RecipientTypeTest, TypeToStringBcc) {
    EXPECT_EQ(Recipient::typeToString(RecipientType::Bcc), "bcc");
}

TEST(RecipientTypeTest, TypeToStringReplyTo) {
    EXPECT_EQ(Recipient::typeToString(RecipientType::ReplyTo), "reply-to");
}

TEST(RecipientTypeTest, TypeFromStringTo) {
    EXPECT_EQ(Recipient::typeFromString("to"), RecipientType::To);
}

TEST(RecipientTypeTest, TypeFromStringCc) {
    EXPECT_EQ(Recipient::typeFromString("cc"), RecipientType::Cc);
}

TEST(RecipientTypeTest, TypeFromStringBcc) {
    EXPECT_EQ(Recipient::typeFromString("bcc"), RecipientType::Bcc);
}

TEST(RecipientTypeTest, TypeFromStringReplyTo) {
    EXPECT_EQ(Recipient::typeFromString("reply-to"), RecipientType::ReplyTo);
}

TEST(RecipientTypeTest, TypeFromStringUnknownDefaultsToTo) {
    EXPECT_EQ(Recipient::typeFromString("unknown"), RecipientType::To);
    EXPECT_EQ(Recipient::typeFromString(""), RecipientType::To);
    EXPECT_EQ(Recipient::typeFromString("TO"), RecipientType::To);
    EXPECT_EQ(Recipient::typeFromString("CC"), RecipientType::To);
}

TEST(RecipientTypeTest, RoundTrip) {
    for (auto t : {RecipientType::To, RecipientType::Cc, RecipientType::Bcc, RecipientType::ReplyTo}) {
        EXPECT_EQ(Recipient::typeFromString(Recipient::typeToString(t)), t);
    }
}
