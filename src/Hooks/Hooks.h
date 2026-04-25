#pragma once

namespace Hooks {
	bool Install();

	/* Cheatsheet:
	*   Virtual function hook
		REL::Relocation<std::uintptr_t> VTABLE{ REL::ID };
		_func = VTABLE.write_vfunc(a_idx, Thunk);
	* 
	*   CALL/JMP instruction hook
	    REL::Relocation<std::uintptr_t> target{ REL::ID, offset };
		if (!REL::make_pattern<"E8">().match(target.address())) {
			return false;
		}
		auto& trampoline = SKSE::GetTrampoline();
		_func = trampoline.write_call<5>(target.address(), &Func);
	*
	*   Prologue Hook
		struct Patch : Xbyak::CodeGenerator
		{
			explicit Patch(uintptr_t OriginalFuncAddr, size_t OriginalByteLength)
			{
				for (size_t i = 0; i < OriginalByteLength; i++)
					db(*reinterpret_cast<uint8_t*>(OriginalFuncAddr + i));

				jmp(qword[rip]);
				dq(OriginalFuncAddr + OriginalByteLength);
			}
		};

		auto& trampoline = SKSE::GetTrampoline();
		const REL::Relocation<std::uintptr_t> target{ REL::ID };

		if (REL::make_pattern<"E9">().match(target.address())) {
			logger::warn("    >Unexpected match - E9. Writing branch instead."sv);
			_func = trampoline.write_branch<5>(target.address(), Func);
			return true;
		}

		Patch p(target.address(), 5);
		p.ready();

		trampoline.write_branch<5>(target.address(), Func);

		auto alloc = trampoline.allocate(p.getSize());
		memcpy(alloc, p.getCode(), p.getSize());

		_func = reinterpret_cast<uintptr_t>(alloc);
	*/
}