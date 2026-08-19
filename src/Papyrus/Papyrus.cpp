#include "Papyrus.h"

namespace Papyrus {

	static std::vector<int> GetVersion(STATIC_ARGS) {
		return { Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2] };
	}

	static bool Bind(VM& a_vm) {
		logger::info("Binding new Papyrus functions..."sv);
		BIND(GetVersion);
		logger::info("  - Bound GetVersion to {}"sv, script);

		logger::info("  - Done!"sv);
		SECTION_SEPARATOR;
		return true;
	}

	static bool RegisterImpl(VM* a_vm) {
		return Bind(*a_vm);
	}

	bool RegisterFunctions() {
		auto* papyrusInterface = SKSE::GetPapyrusInterface();
		if (!papyrusInterface) {
			return false;
		}
		return papyrusInterface->Register(RegisterImpl);
	}
}