#pragma once

namespace Serialization
{
	namespace SerializationManager
	{
		class ObjectManager;

		class Serializable
		{
		public:
			virtual bool Save(SKSE::SerializationInterface* a_intfc) = 0;
			virtual bool Load(SKSE::SerializationInterface* a_intfc) = 0;
			virtual void Revert(SKSE::SerializationInterface* a_intfc) = 0;

			template <typename T>
			bool RegisterForSerialization(T* object, uint32_t type) {
				logger::info("  >Registering for Save/Load events..."sv);
				auto* manager = ObjectManager::GetSingleton();
				if (!manager) {
					logger::critical("    >Failed to get Serialization Object Manager."sv);
					return false;
				}

				auto pointer = std::unique_ptr<Serializable>(object);
				manager->RegisterObject(pointer, type);
				return true;
			}
		};

		class ObjectManager : public REX::Singleton<ObjectManager>
		{
		public:
			bool Save(SKSE::SerializationInterface* a_intfc);
			bool Load(SKSE::SerializationInterface* a_intfc);
			bool Revert(SKSE::SerializationInterface* a_intfc);

			void RegisterObject(Serializable* a_newObject, uint32_t a_recordType);

		private:
			std::unordered_map<uint32_t, Serializable*> recordObjectMap{};
		};

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
			if (!a_intfc->ReadRecordData(a_str.data(), static_cast<std::uint32_t>(size))) {
				return false;
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
}