#include <Windows.h>
#include <TlHelp32.h>

BOOL
WINAPI
InjectDll(
	_In_ WCHAR*   TargetProcess,
	_In_ LPCWSTR  Dll
)
{
	PROCESSENTRY32W pe = { 0 };
	pe.dwSize = sizeof( PROCESSENTRY32W );
	HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPALL, pe.th32ProcessID );
	if ( hSnapshot == INVALID_HANDLE_VALUE ) return FALSE;
	if ( Process32FirstW( hSnapshot, &pe ) )
	{
		do
		{
			if ( _wcsicmp( pe.szExeFile, TargetProcess ) == 0 )
			{
				SIZE_T sz;
				HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID );
				if ( !hProcess ) return FALSE;

				PVOID pMemory = VirtualAllocEx( hProcess, NULL, ( wcslen( Dll ) + 1 ) * sizeof( WCHAR ), MEM_COMMIT, PAGE_EXECUTE_READWRITE );
				if ( pMemory == NULL )
				{
					CloseHandle( hProcess );
					CloseHandle( hSnapshot );
					return FALSE;
				}

				if ( !WriteProcessMemory( hProcess, pMemory, Dll, ( wcslen( Dll ) + 1 ) * sizeof( WCHAR ), &sz ) )
				{
					VirtualFreeEx( hProcess, pMemory, ( wcslen( Dll ) + 1 ) * sizeof( WCHAR ), MEM_RELEASE );
					CloseHandle( hProcess );
					CloseHandle( hSnapshot );
					return FALSE;
				}

				HANDLE hThread = CreateRemoteThread( hProcess, NULL, 0, ( PTHREAD_START_ROUTINE )GetProcAddress( GetModuleHandleW( L"Kernel32.dll" ), "LoadLibraryW" ), pMemory, 0, NULL );
				if ( hThread == NULL )
				{
					VirtualFreeEx( hProcess, pMemory, ( wcslen( Dll ) + 1 ) * sizeof( WCHAR ), MEM_RELEASE );
					CloseHandle( hProcess );
					CloseHandle( hSnapshot );
					return FALSE;
				}

				WaitForSingleObject( hThread, INFINITE );
				VirtualFreeEx( hProcess, pMemory, ( wcslen( Dll ) + 1 ) * sizeof( WCHAR ), MEM_RELEASE );
				
				if ( hProcess ) CloseHandle( hProcess );
				if ( hThread )  CloseHandle( hThread );
				break;
			}
		} while ( Process32NextW( hSnapshot, &pe ) );
	}

	CloseHandle( hSnapshot );
	return TRUE;
}

INT
WINAPI
wWinMain(
	_In_	 HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_     PWSTR     szCmdLine,
	_In_     INT       nShowCmd
)
{
	InjectDll( L"MyApp.exe", L"D:\\Path\\To\\DLL" );
	return 0;
}