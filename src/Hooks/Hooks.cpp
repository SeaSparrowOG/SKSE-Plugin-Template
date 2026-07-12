#include "Hooks.h"

namespace Hooks
{
	bool Install() {
		logger::info("Installing hooks..."sv);
		logger::info("  - Done!"sv);
		return true;
	}
}