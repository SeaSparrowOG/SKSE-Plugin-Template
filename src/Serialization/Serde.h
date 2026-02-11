#pragma once

namespace Serialization
{
	constexpr std::uint32_t Version = 1;
	inline constexpr std::uint32_t ID = 'TRJT';
	static_assert(Plugin::NAME == "TemplateProject"sv || ID != 'TRJT', "Make sure to use a unique ID for serialization.");

	void SaveCallback(SKSE::SerializationInterface* a_intfc);
	void LoadCallback(SKSE::SerializationInterface* a_intfc);
	void RevertCallback(SKSE::SerializationInterface* a_intfc);

	/// <summary>
	/// Important notes:
	/// * If Save, Load, or Revert fail, then the application state is considered corrupted and irrecoverable. Thus, the game closes.
	/// * When calling Register, provide an a_id that is unique for EACH class. If a duplicate insertion is attempted, the game closes.
	/// * Make sure to provide logging for each Save/Load/Revert failure as details. ObjectManager does note some things.
	/// * A pointer to a Serializable class is stored in the ObjectManager singleton after it is registered. To move/delete/whatever, first Unregister it from the ObjectManager.
	/// * Ideally, also inherit from REX::Singleton to deal with lifetime issues.
	/// </summary>
	class Serializable
	{
	public:
		virtual bool Save(SKSE::SerializationInterface* a_intfc) = 0;
		virtual bool Load(SKSE::SerializationInterface* a_intfc) = 0;
		virtual bool Revert(SKSE::SerializationInterface* a_intfc);

		bool Register(std::uint32_t a_id);
		std::uint32_t GetSerializationID() const;
	private:
		std::uint32_t serdeID{ 0 };
	};

	class ObjectManager : public REX::Singleton<ObjectManager>
	{
	public:
		bool Save(SKSE::SerializationInterface* a_intfc);
		bool Load(SKSE::SerializationInterface* a_intfc);
		bool Revert([[maybe_unused]] SKSE::SerializationInterface* a_intfc);

		void RegisterObject(Serializable& a_newObject);
		void UnRegisterObject(Serializable& a_object);

	private:
		std::unordered_map<uint32_t, Serializable*> recordObjectMap{};
	};
}