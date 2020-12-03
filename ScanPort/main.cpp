#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Ws2_32.lib")
#include <sstream>
#include <windows.h>
#include <tchar.h>
#include "commctrl.h"
#include <string>
#include <map>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
using namespace std;


#define IDM_CODE_SAMPLES  1
#define TCPIP_OWNING_MODULE_SIZE 16

typedef struct PORT_INFO
{
    DWORD Port;
    DWORD PID;
    wstring Name;
}; 

typedef struct PORT_INFO_VECTOR
{
    std::vector<PORT_INFO> PortInfo;
}; 

typedef enum
{
    TCP_TABLE_BASIC_LISTENER,
    TCP_TABLE_BASIC_CONNECTIONS,
    TCP_TABLE_BASIC_ALL,
    TCP_TABLE_OWNER_PID_LISTENER,
    TCP_TABLE_OWNER_PID_CONNECTIONS,
    TCP_TABLE_OWNER_PID_ALL,
    TCP_TABLE_OWNER_MODULE_LISTENER,
    TCP_TABLE_OWNER_MODULE_CONNECTIONS,
    TCP_TABLE_OWNER_MODULE_ALL
} TCP_TABLE_CLASS, * PTCP_TABLE_CLASS;

typedef DWORD(WINAPI* PFNGetExtendedTcpTable)
(
    void* pTcpTable,
    PDWORD pdwSize,
    BOOL bOrder,
    ULONG ulAf,
    TCP_TABLE_CLASS TableClass,
    ULONG Reserved
);

typedef struct _MIB_TCPROW_OWNER_MODULE
{
    DWORD         dwState;
    DWORD         dwLocalAddr;
    DWORD         dwLocalPort;
    DWORD         dwRemoteAddr;
    DWORD         dwRemotePort;
    DWORD         dwOwningPid;
    LARGE_INTEGER liCreateTimestamp;
    ULONGLONG     OwningModuleInfo[TCPIP_OWNING_MODULE_SIZE];

} MIB_TCPROW_OWNER_MODULE, * PMIB_TCPROW_OWNER_MODULE;

typedef struct
{
    DWORD                   dwNumEntries;
    MIB_TCPROW_OWNER_MODULE table[1];
} MIB_TCPTABLE_OWNER_MODULE, * PMIB_TCPTABLE_OWNER_MODULE;

LONG WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
void GetProccesesNamesAndPIDs(map<DWORD, wstring>& mNamesAndPIDs);
bool GetPortInfo(PORT_INFO_VECTOR* info);
DWORD StrToInt(wchar_t* s);
std::string int_to_string(int v);
HINSTANCE hinstance;
PORT_INFO_VECTOR pi;
DWORD countPorts = 0;
wchar_t headers[3][22] = { L"Port",L"PID",L"Name Process" };
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    hinstance = hInstance;
    HWND hwnd;
    MSG msg; 
    WNDCLASS w; 

    memset(&w, 0, sizeof(WNDCLASS));
    w.style = CS_HREDRAW | CS_VREDRAW;
    w.lpfnWndProc = WndProc; 
    w.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    w.hInstance = hInstance;
    w.hbrBackground = (HBRUSH)(WHITE_BRUSH);
    w.lpszClassName = L"My Class";
    RegisterClass(&w);
    
    hwnd = CreateWindow(L"My Class", L"ScanPort",
        WS_OVERLAPPEDWINDOW&(~WS_MAXIMIZEBOX) ^ WS_THICKFRAME , 500, 300, 500, 500, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow); 
    UpdateWindow(hwnd);          

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
LONG WINAPI WndProc(HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam)
{
    
    InitCommonControls();
    PAINTSTRUCT ps;
    static HWND hBtnCheckAllPorts;
    static HWND hListView;
    static HWND hBtnCheckPort;
    static HWND hBtnRemove;
    static HWND hEdt;
    static HDC hdc;
    LVCOLUMN lvc;
    LVITEM lvi;
    wchar_t StrPort[10];
    DWORD port = 0;        
    switch (Message)
    {
   
    case WM_CREATE:
       

        hBtnCheckAllPorts = CreateWindow(L"button", L"CheckAllPorts",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            0, 20, 100, 20, hwnd, 0, hinstance, NULL);
        ShowWindow(hBtnCheckAllPorts, SW_SHOWNORMAL);

        hBtnCheckPort = CreateWindow(L"button", L"CheckPort",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            0, 60, 100, 20, hwnd, 0, hinstance, NULL);
        ShowWindow(hBtnCheckAllPorts, SW_SHOWNORMAL);
        hEdt = CreateWindow(L"edit", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_RIGHT |ES_NUMBER,
            0, 40, 100, 20, hwnd, 0, hinstance, NULL);
        ShowWindow(hEdt, SW_SHOWNORMAL);
        hBtnRemove = CreateWindow(L"button", L"RemoveList",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            0, 100, 100, 20, hwnd, 0, hinstance, NULL);
        ShowWindow(hBtnCheckAllPorts, SW_SHOWNORMAL);
        
        GetPortInfo(&pi);
        countPorts = pi.PortInfo.size();

        hListView = CreateWindow(WC_LISTVIEW, L"Good",
            LVS_REPORT | WS_CHILD | WS_VISIBLE | WS_HSCROLL ,
            100, 20, 270, 600,
            hwnd, (HMENU)1010, hinstance, 0);

        if (hListView == NULL)
            MessageBox(hwnd, L"", L"", MB_OK);

        lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = 90;

        lvc.iSubItem = 0;
        lvc.pszText = headers[0];
        ListView_InsertColumn(hListView, 0, &lvc);

        lvc.iSubItem = 1;
        lvc.pszText = headers[1];
        ListView_InsertColumn(hListView, 1, &lvc);

        lvc.iSubItem = 2;
        lvc.pszText = headers[2];
        ListView_InsertColumn(hListView, 2, &lvc);

        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps); 
        TextOut(hdc, 0, 0, L"Проверьте все порты или конкретный", 35);
        
        break;
    case WM_COMMAND:
        if (lparam == (LPARAM)hBtnCheckAllPorts)
        {  
            memset(&lvi, 0, sizeof(lvi));
            lvi.mask = LVIF_TEXT | LVIF_TEXT;

            if (true) {
                
                std::vector<PORT_INFO>& vPortInfo = pi.PortInfo;
                for (UINT i = 0; i < vPortInfo.size(); i++)
                {
                    lvi.iItem = i;
                    lvi.iSubItem = 0;
                    ListView_InsertItem(hListView, &lvi);

                    lvi.iItem = i;
                    lvi.iSubItem = 1;
                    ListView_InsertItem(hListView, &lvi);

                    lvi.iItem = i;
                    lvi.iSubItem = 2;
                    ListView_InsertItem(hListView, &lvi);
                }
                for (UINT i = 0; i < vPortInfo.size(); i++) {
                    auto s = std::to_wstring(vPortInfo[i].Port);
                    ListView_SetItemText(hListView, i, 0, (LPWSTR)s.c_str());

                    s = std::to_wstring(vPortInfo[i].PID);
                    ListView_SetItemText(hListView, i, 1, (LPWSTR)s.c_str());

                    ListView_SetItemText(hListView, i, 2, (LPWSTR)vPortInfo[i].Name.c_str());
                }
               
                int a = countPorts;
                TCHAR buf[10] = { 0 };
                _stprintf_s(buf, TEXT("%d"), a);
               
                TextOut(hdc, 280, 0, L"Number of used ports:", 20);
                TextOut(hdc, 425,0 , buf, 2);
            }
            EnableWindow(hBtnCheckAllPorts, false);
            
        }

        if (lparam == (LPARAM)hBtnCheckPort)
        {
                        
            bool flag = false;
            int len = GetWindowText(hEdt, StrPort, 40);            
            port = StrToInt(StrPort);
            if (port <=0 && port >=65535)
            {
                MessageBox(GetActiveWindow(), L"Введенный вами порт не существует", L"Error", MB_ICONERROR);
                break;
            }
            std::vector<PORT_INFO>& vPortInfo = pi.PortInfo;
            for (UINT i = 0; i < vPortInfo.size(); i++)
            {
                if (port == vPortInfo[i].Port) {                                                           
                    MessageBox(GetActiveWindow(), L"Выбранный порт занят" , L"Error", MB_ICONERROR);
                    flag = true;
                    break;
                }
            }
            if (!flag) MessageBox(GetActiveWindow(), L"Выбранный порт не занят", L"Info", MB_ICONWARNING);                                                             
        }
        if (lparam == (LPARAM)hBtnRemove)
        {
            ListView_DeleteAllItems(hListView);
            ListView_Update(hListView, 0);
            EnableWindow(hBtnCheckAllPorts, true);
        }
        UpdateWindow(hListView);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, Message, wparam, lparam);
    }
    return 0;
}
std::string int_to_string(int v) {
    std::stringstream ss;
    ss << v;
    return ss.str();
}

DWORD StrToInt(wchar_t* s)
{
    int temp = 0; // число
    int i = 0;
    int sign = 0; // знак числа 0- положительное, 1 - отрицательное
    if (s[i] == '-')
    {
        sign = 1;
        i++;
    }
    while (s[i] >= 0x30 && s[i] <= 0x39)
    {
        temp = temp + (s[i] & 0x0F);
        temp = temp * 10;
        i++;
    }
    temp = temp / 10;
    if (sign == 1)
        temp = -temp;
    return(temp);
}

bool GetPortInfo(PORT_INFO_VECTOR * info)
{
    // загрузка dll
    static HINSTANCE hDLL = NULL;
    if (hDLL == NULL) {
        hDLL = LoadLibrary(L"iphlpapi.dll");
    }

    if (hDLL == NULL) {
        MessageBox(GetActiveWindow(), L"Ошибка при загрузке iphlpapi.dll", L"Error", MB_ICONERROR);        
        return false;
    }

    // получение указателя на функцию GetExtendedTcpTable
    PFNGetExtendedTcpTable pGetExtendedTcpTable;
    
    pGetExtendedTcpTable = (PFNGetExtendedTcpTable)GetProcAddress(hDLL, "GetExtendedTcpTable");

    if (!pGetExtendedTcpTable)
    {
        MessageBox(GetActiveWindow(), L"Ошибка при загрузке iphlpapi.dll", L"Error", MB_ICONERROR);
        return false;
    }


    PMIB_TCPTABLE_OWNER_MODULE pTcpTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;

    dwSize = sizeof(MIB_TCPTABLE_OWNER_MODULE);
    pTcpTable = (MIB_TCPTABLE_OWNER_MODULE*)MALLOC(dwSize);

    if (pTcpTable == NULL)
    {
        MessageBox(GetActiveWindow(), L"Ошибка при выделении памяти для pTcpTable", L"Error", MB_ICONERROR);
        return false;
    }

    //первый вызов GetTcpTable, для получения нужного размера dwSize
    if ((dwRetVal = pGetExtendedTcpTable(pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_LISTENER, 0)) == ERROR_INSUFFICIENT_BUFFER)
    {
        FREE(pTcpTable);
        pTcpTable = (MIB_TCPTABLE_OWNER_MODULE*)MALLOC(dwSize);
        if (pTcpTable == NULL)
        {
            MessageBox(GetActiveWindow(), L"Ошибка при выделении памяти для pTcpTable после вызова функции GetExtendedTcpTable", L"Error", MB_ICONERROR);
            return false;
        }
    }

    //второй вызов GetTcpTable, чтобы получить
    //фактические данные, которые нам нужны
    if ((dwRetVal = pGetExtendedTcpTable(pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_LISTENER, 0)) == NO_ERROR)
    {
        map<DWORD, wstring> mNamesAndPIDs;
        GetProccesesNamesAndPIDs(mNamesAndPIDs);

        for (UINT i = 0; i < pTcpTable->dwNumEntries; i++)
        {
            USHORT localport = ntohs((u_short)pTcpTable->table[i].dwLocalPort);
            DWORD pid = pTcpTable->table[i].dwOwningPid;

            PORT_INFO pi;
            pi.Port = localport;
            pi.PID = pid;
            pi.Name = mNamesAndPIDs[pid];

            info->PortInfo.push_back(pi);
        }
    }
    else
    {
        MessageBox(GetActiveWindow(), L"Ошибка при вызове функции GetExtendedTcpTable", L"Error", MB_ICONERROR);
        return false;
    }

    return true;
}
void GetProccesesNamesAndPIDs(map<DWORD, wstring>& mNamesAndPIDs)
{
    mNamesAndPIDs.clear();

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        MessageBox(GetActiveWindow(), L"Ошибка при вызове функции CreateToolhelp32Snapshot", L"Error", MB_ICONERROR);           
    }

    if (Process32First(hProcessSnap, &pe32))
    {
        do
        {
            mNamesAndPIDs[pe32.th32ProcessID] = pe32.szExeFile;
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
   
}
