#include "papyrus.h"

namespace Papyrus
{
	std::vector<int> GetVersion(STATIC_ARGS) {
		return { 1, 1, 1 };
	}

	void Bind(VM& a_vm) {
		BIND(GetVersion);
	}

	bool RegisterFunctions(VM* a_vm) {
		Bind(*a_vm);
		return true;
	}
}
