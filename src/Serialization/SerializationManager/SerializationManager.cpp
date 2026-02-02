#include "SerializationManager.h"

namespace Serialization::SerializationManager
{
	bool ObjectManager::Save(SKSE::SerializationInterface* a_intfc) {
		for (auto& obj : recordObjectMap) {
			if (!obj.second->Save(a_intfc)) {
				logger::error("Serialization failed."sv);
				return false;
			}
		}
		return true;
	}

	bool ObjectManager::Load(SKSE::SerializationInterface* a_intfc) {
		std::uint32_t type;
		std::uint32_t version;
		std::uint32_t length;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (recordObjectMap.contains(type)) {
				if (!recordObjectMap.at(type)->Load(a_intfc)) {
					logger::error("De-Serialization failed."sv);
					return false;
				}
			}
			else {
				return false;
			}
		}
		return true;
	}

	bool ObjectManager::Revert(SKSE::SerializationInterface* a_intfc) {
		for (auto& obj : recordObjectMap) {
			obj.second->Revert(a_intfc);
		}
		return true;
	}

	void ObjectManager::RegisterObject(Serializable* a_newObject, uint32_t a_recordType) {
#ifndef NDEBUG
		if (recordObjectMap.contains(a_recordType)) {
			SKSE::stl::report_and_fail(fmt::format("Tried to serialize object twice."sv));
		}
#endif
		recordObjectMap.emplace(a_recordType, a_newObject);
	}
}