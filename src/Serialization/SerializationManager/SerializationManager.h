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
	}
}