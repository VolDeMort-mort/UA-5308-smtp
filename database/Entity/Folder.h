#pragma once

#include <cstdint>
#include <optional>
#include <string>

struct Folder
{
	// FIX: should be std::optional<int64_t> parent_id;
	std::optional<int64_t> id;
	int64_t user_id;
	std::optional<int64_t> parent_id;
	std::string name;
	int64_t next_uid = 1;
	bool is_subscribed = false;

	bool operator==(const Folder& other)
	{
		return this->id == other.id && this->user_id == other.user_id && this->parent_id == other.parent_id &&
			   this->name == other.name && this->next_uid == other.next_uid &&
			   this->is_subscribed == other.is_subscribed;
	}
};