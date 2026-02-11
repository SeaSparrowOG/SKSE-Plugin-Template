#include "Serde.h"

namespace
{
	/// <summary>
	/// Debug tool. When encountering unexpected RecordTypes, converts them to a readable string (HDEC, STEN, etc).
	/// </summary>
	/// <param name="a_typeCode">The unexpected record type.</param>
	/// <returns>The unexpected record type as a string.</returns>
	inline std::string DecodeTypeCode(std::uint32_t a_typeCode)
	{
		std::string result(4, '\0');

		// Extract bytes from most significant to least
		result[0] = static_cast<char>((a_typeCode >> 24) & 0xFF);
		result[1] = static_cast<char>((a_typeCode >> 16) & 0xFF);
		result[2] = static_cast<char>((a_typeCode >> 8) & 0xFF);
		result[3] = static_cast<char>(a_typeCode & 0xFF);

		return result;
	}

	/// <summary>
	/// Helper function. Encodes a string into the interface.
	/// </summary>
	/// <param name="a_intfc">The serialization interface provided by SKSE.</param>
	/// <param name="a_str">The string to serialize.</param>
	/// <returns>True if encoding is successful, false otherwise.</returns>
	inline static bool WriteString(SKSE::SerializationInterface* a_intfc,
		const std::string& a_str)
	{
		std::size_t size = a_str.length() + 1;
		return a_intfc->WriteRecordData(size) && a_intfc->WriteRecordData(a_str.data(), static_cast<std::uint32_t>(size));
	}

	/// <summary>
	/// Helper function. Decodes a string from the interface, and stores it in a given variable.
	/// </summary>
	/// <param name="a_intfc">The serialization interface provided by SKSE.</param>
	/// <param name="a_str">The result is stored here.</param>
	/// <returns>True if successful, false otherwise.</returns>
	inline static bool ReadString(SKSE::SerializationInterface* a_intfc,
		std::string& a_str)
	{
		std::size_t size = 0;
		if (!a_intfc->ReadRecordData(size)) {
			return false;
		}
		a_str.reserve(size);
		a_str.resize(size);
		if (!a_intfc->ReadRecordData(a_str.data(), static_cast<std::uint32_t>(size))) {
			return false;
		}
		if (!a_str.empty() && a_str.back() == '\0') {
			a_str.pop_back();
		}
		return true;
	}

	/// <summary>
	/// Helper function. Fetches the form found inside the serialization interface, and resolves it.
	/// </summary>
	/// <typeparam name="T">Cast the form as T</typeparam>
	/// <param name="a_intfc">The serialization interface provided by SKSE.</param>
	/// <returns>A pointer to T* if found, nullptr otherwise.</returns>
	template <typename T>
	inline static T* GetFormFromInterface(SKSE::SerializationInterface* a_intfc) {
		RE::FormID oldID = 0;
		if (!a_intfc->ReadRecordData(oldID)) {
			return nullptr;
		}
		RE::FormID newID = 0;
		if (!a_intfc->ResolveFormID(oldID, newID)) {
			return nullptr;
		}
		return RE::TESForm::LookupByID<T>(newID);
	}
}

namespace Serialization {
	void SaveCallback(SKSE::SerializationInterface* a_intfc) {
		logger::info("Starting save..."sv);
		auto* serdeManager = ObjectManager::GetSingleton();
		if (!serdeManager) {
			logger::critical("  >Failed to get internal serialization manager."sv);
			SKSE::stl::report_and_fail(fmt::format("{}:  Failed to save. Check the log for more information.", Plugin::NAME));
		}
		if (!serdeManager->Save(a_intfc)) {
			SKSE::stl::report_and_fail(fmt::format("{}:  Failed to save. Check the log for more information.", Plugin::NAME));
		}
		logger::info("  >Save successful."sv);
	}

	void LoadCallback(SKSE::SerializationInterface* a_intfc) {
		logger::info("Starting load..."sv);
		auto* serdeManager = ObjectManager::GetSingleton();
		if (!serdeManager) {
			logger::critical("  >Failed to get internal serialization manager."sv);
			SKSE::stl::report_and_fail(fmt::format("{}:  Failed to load. Check the log for more information.", Plugin::NAME));
		}
		if (!serdeManager->Load(a_intfc)) {
			SKSE::stl::report_and_fail(fmt::format("{}:  Failed to load. Check the log for more information.", Plugin::NAME));
		}
		logger::info("  >Load successful."sv);
	}

	void RevertCallback(SKSE::SerializationInterface* a_intfc) {
		logger::info("Starting revert..."sv);
		auto* serdeManager = ObjectManager::GetSingleton();
		if (!serdeManager) {
			logger::critical("  >Failed to get internal serialization manager."sv);
			SKSE::stl::report_and_fail(fmt::format("{}:  Failed to revert. Check the log for more information.", Plugin::NAME));
		}
		if (!serdeManager->Revert(a_intfc)) {
			SKSE::stl::report_and_fail(fmt::format("{}:  Failed to revert. Check the log for more information.", Plugin::NAME));
		}
		logger::info("  >Revert successful."sv);
	}

	bool ObjectManager::Save(SKSE::SerializationInterface* a_intfc) {
		bool success = true;
		if (recordObjectMap.empty()) {
			return success;
		}

		for (auto& obj : recordObjectMap) {
			bool serializableSuccess = obj.second && obj.second->Save(a_intfc);
			if (!serializableSuccess) {
				logger::critical("  >Serialization error reported for object: {}"sv, obj.second ? 
					DecodeTypeCode(obj.second->GetSerializationID()) : 
					"NULL");
			}
			success &= serializableSuccess;
		}
		return success;
	}

	bool ObjectManager::Load(SKSE::SerializationInterface* a_intfc) {
		bool success = true;
		if (recordObjectMap.empty()) {
			return success;
		}

		std::uint32_t type;
		std::uint32_t version;
		std::uint32_t length;
		auto end = recordObjectMap.end();

		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			auto it = recordObjectMap.find(type);
			if (it != end) {
				bool serializableSuccess = it->second && it->second->Load(a_intfc);
				if (!serializableSuccess) {
					logger::critical("  >Serialization error reported for object: {}"sv, DecodeTypeCode(type));
				}
				success &= serializableSuccess;
			}
		}
		return success;
	}

	bool ObjectManager::Revert(SKSE::SerializationInterface* a_intfc) {
		if (recordObjectMap.empty()) {
			return true;
		}

		bool success = true;
		for (auto& obj : recordObjectMap) {
			bool serializableSuccess = obj.second && obj.second->Revert(a_intfc);
			if (!serializableSuccess) {
				logger::critical("  >Serialization error reported for object: {}"sv, obj.second ?
					DecodeTypeCode(obj.second->GetSerializationID()) :
					"NULL");
			}
			success &= serializableSuccess;
		}
		return success;
	}

	void ObjectManager::RegisterObject(Serializable& a_newObject) {
		auto recordType = a_newObject.GetSerializationID();
		if (recordObjectMap.contains(recordType)) {
			SKSE::stl::report_and_fail(
				fmt::format("Duplicate serialization registration for record type {}", DecodeTypeCode(recordType)));
		}
		recordObjectMap.emplace(recordType, &a_newObject);
	}

	void ObjectManager::UnRegisterObject(Serializable& a_object) {
		auto it = recordObjectMap.find(a_object.GetSerializationID());
		if (it != recordObjectMap.end()) {
			recordObjectMap.erase(it);
		}
	}

	bool Serializable::Register(std::uint32_t a_id) {
		auto* manager = ObjectManager::GetSingleton();
		if (!manager) {
			logger::critical("Critical error registering serializable form: Internal ObjectManager returned invalid pointer."sv);
			return false;
		}

		serdeID = a_id;
		manager->RegisterObject(*this);
		return true;
	}

	std::uint32_t Serializable::GetSerializationID() const { return serdeID; }

	bool Serializable::Revert([[maybe_unused]] SKSE::SerializationInterface* a_intfc) {
		return true;
	}
}