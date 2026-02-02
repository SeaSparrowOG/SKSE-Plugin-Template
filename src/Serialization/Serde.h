#pragma once

namespace Serialization
{
	constexpr std::uint32_t Version = 1;
	inline constexpr std::uint32_t ID = 'TRJT';

	void SaveCallback(SKSE::SerializationInterface* a_intfc);
	void LoadCallback(SKSE::SerializationInterface* a_intfc);
	void RevertCallback(SKSE::SerializationInterface* a_intfc);
}