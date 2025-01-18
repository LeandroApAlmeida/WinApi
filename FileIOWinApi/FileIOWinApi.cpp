
/*
    Neste programa de exemplo, vou demonstrar como gravar o texto "Ola mundo!" em um arquivo, e depois
    ler este mesmo arquivo e exibir seu conteúdo numa caixa de diálogo usando apenas chamadas à API do
    Sistema Operacional Windows. O objetivo do programa é fazer a demonstração de como estas operações
    acontecem por "debaixo do capô" das linguagens de programação, que disponibilizam tais recursos na
    forma de classes como FileReader, FileWriter, etc, que constituem abstrações de "alto nível" destas
    linguagens. É muito importante que fique claro que ao nível da API do Windows, ainda estamos manipulando
    abstrações de alto nível, na forma das chamadas a funções em bibliotecas de vínculo dinâmico (Dynamic
    Link Library - DLL), que estão muito longe de refletir a verdadeira complexidade que é o acesso ao
    hardware real da máquina. São diversas camadas de abstração que separam as chamadas à API do Windows
    do hardware, mas em termos de Sistemas Operacionais, as chamadas de sistema são o mais próximo que
    conseguimos chegar deste. Por API do Windows (Windows API), entenda-se como um conjunto de arquivos DLL,
    entre elas Kernel32.dll, user32.dll, gdi32.dll, shell32.dll, etc, que fornecem acesso à todas as
    funcionalidades do sistema de modo controlado. No Linux temos algo semelhante com as system calls.

    O Sistema Windows, gerencia toda a complexidade de acessar um dispositivo físico, como ler seu estado,
    alterar valores em registradores do mesmo, carregar os drivers necessários, etc, etc, etc. Depois
    de resolver todas estas etapas de grau de complexidade bastante alto, e muito enfadonhas para um ser
    humano ficar se ocupando delas toda vez que apenas quer abrir uma planilha de finanças, o Kernel do
    sistema Windows devolve os bytes do arquivo, ou os recebe, por meio de chamadas de funções com nomes muito
    sugestivos como "ReadFile" e "WriteFile" disponíveis em dll's (no caso, kernel32.dll), que no contexto
    do sistema Windows, nada mais são do que executáveis comuns, assim como os arquivos .exe, porém elas
    funcionam como componentes separados fisicamente do executável principal do programa, se é que podemos
    dizer assim de arquivos binários, e normalmente são carregadas na área de memória do programa apenas
    quando o executável precisa de alguma funcionalidade disponível nelas. É como se seu fígado ficasse fora
    do seu abdôme, e você só o recolocasse no lugar quando precisasse dele, pois o fígado faz parte do seu
    corpo, e sem ele você não teria as funções necessárias para manter-se vivo. A analogia foi horrível, eu
    sei, mas é aproximadamente dessa forma que estou carregando as DLL's do Windows que vou usar para ler e
    gravar um arquivo via chamada à API do Windows e exibir o texto do arquivo numa caixa de diálogo padrão.

    Se você for alguém que quer desenvolver um compilador para Windows, para implementar uma nova linguagem
    de programação, ou apenas alguém que reflete sobre coisas aleatórias como eu, e quiser consultar a
    documentação online da API do Windows, ela está disponível em português nestes endereços:

    https://learn.microsoft.com/pt-br/windows/win32/apiindex/windows-api-list?form=MG0AV3

    https://learn.microsoft.com/pt-br/windows/apps/api-reference/?form=MG0AV3

*/


#include <windows.h>
#include <stdio.h>
#include <wchar.h>


/*
    Declaração dos tipos de ponteiros de função para acesso em tempo de execução às funções de leitura
    e escrita de um arquivo. Todas estas funções estão disponíveis por meio da DLL kernel32.dll.
*/


// ReadFile_t: Tipo de ponteiro para a função de leitura do arquivo.

typedef BOOL(WINAPI* ReadFile_t)(
    HANDLE hFile,                  // Identificador do arquivo a ser lido (path do arquivo).
    LPVOID lpBuffer,               // Ponteiro para o buffer que receberá os dados lidos do arquivo.
    DWORD nNumberOfBytesToRead,    // Número de bytes a serem lidos do arquivo.
    LPDWORD lpNumberOfBytesRead,   // Ponteiro para a variável que receberá o número de bytes lidos.
    LPOVERLAPPED lpOverlapped      // Estrutura OVERLAPPED usada para operações assíncronas.
    );


// WriteFile_t: Tipo de ponteiro para a função de escrita do arquivo.
typedef BOOL(WINAPI* WriteFile_t)(
    HANDLE hFile,                  // Identificador do arquivo a ser escrito (path do arquivo).
    LPCVOID lpBuffer,              // Ponteiro para o buffer com dados a serem escritos no arquivo.
    DWORD nNumberOfBytesToWrite,   // Número de bytes a serem escritos no arquivo.
    LPDWORD lpNumberOfBytesWritten,// Ponteiro para a variável que receberá o número de bytes escritos.
    LPOVERLAPPED lpOverlapped      // Estrutura OVERLAPPED usada para operações assíncronas.
    );


// CreateFileW_t: Tipo de ponteiro para a função de abertura/criação do arquivo.

typedef HANDLE(WINAPI* CreateFileW_t)(
    LPCWSTR lpFileName,             // Nome do arquivo a ser criado ou aberto.
    DWORD dwDesiredAccess,          // Especifica o tipo de acesso ao arquivo (leitura, gravação ou ambos).
    DWORD dwShareMode,              // Especifica o modo de compartilhamento do arquivo.                                     
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // Especifica os atributos de segurança do arquivo.
    DWORD dwCreationDisposition,    // Especifica a ação a ser tomada ao criar/abrir o arquivo.
    DWORD dwFlagsAndAttributes,     // Especifica atributos e bandeiras do arquivo.
    HANDLE hTemplateFile            // Handle que fornece os atributos a serem aplicados ao arquivo criado.
    );


/*
    Declaração dos tipos de ponteiros de função para acesso em tempo de execução às funções de criação e
    exibição de uma caixa de diálogo padrão do Windows Forms. Este diálogo já "vem pronto", de forma que
    não precisamos criar nenhum controle dentro dele e manipular eventos.
*/


// MessageBoxW_t: Tipo de ponteiro para a função de caixa de diálogo.

typedef int (WINAPI* MessageBoxW_t)(
    HWND,            // Handle para uma janela proprietária (pode ser NULL).
    LPCWSTR,         // Texto da mensagem a ser exibida na caixa de diálogo.
    LPCWSTR,         // Texto do título da caixa de diálogo.
    UINT             // Especifica o estilo da caixa de diálogo (botões, ícone).
    );




/// <summary>
/// Este método exibe uma caixa de diálogo padrão do Windows Forms, via chamada à API do Windows,
/// mostrando nela o texto que foi passado no parâmetro "message".
/// </summary>
/// <param name="message">Texto a ser exibido na caixa de mensagem</param>
BOOL showMessageBox(const wchar_t* message) {

    // Carrega user32.dll para a área de memória do programa.
    HMODULE hUser32 = LoadLibraryEx(L"user32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

    if (hUser32 == NULL) {
        // Se houve erro ao carregar user32.dll, retorna false.
        wprintf(L"Erro ao carregar user32.dll.\n");
        return FALSE;
    }

    // Obtém a função MessageBoxW de user32.dll para ter acesso à caixa de diálogo.
    MessageBoxW_t pMessageBoxW = (MessageBoxW_t)GetProcAddress(hUser32, "MessageBoxW");

    if (pMessageBoxW == NULL) {
        // Se houve erro ao obter a função MessageBoxW, retorna false.
        wprintf(L"Erro ao obter endereço de MessageBoxW.\n");
        FreeLibrary(hUser32);
        return FALSE;
    }

    // Exibe a caixa de diálogo com a mensagem passada por parâmetro ao método.
    int btn = pMessageBoxW(NULL, message, L"Texto do arquivo", MB_OKCANCEL | MB_ICONINFORMATION);

    // Identifica o botão que foi clicado.
    if (btn == 1) {
        wprintf(L"Você clicou no botão OK.\n");
    }
    else {
        wprintf(L"Você clicou no botão Cancelar.\n");
    }

    FreeLibrary(hUser32);

    return TRUE;

}


/// <summary>
/// Este método lê o arquivo com o path passado como parâmetro, e exibe o texto recuperado
/// do arquivo numa caixa de diálogo padrão do Windows, tudo via chamadas à API do Windows.
/// </summary>
/// <param name="filename">Path do arquivo a ser lido.</param>
/// <returns>True, se o arquivo foi aberto. False se houve erro ao abrir o arquivo.</returns>
BOOL readFile(const wchar_t* filename) {

    // Carrega kernel32.dll para a área de memória do programa.
    HMODULE hKernel32 = LoadLibraryEx(L"kernel32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

    if (hKernel32 == NULL) {
        // Se houve erro ao carregar kernel32.dll, retorna false.
        wprintf(L"Erro ao carregar kernel32.dll.\n");
        return FALSE;
    }

    // Obtém os endereços das funções CreateFileW e ReadFile de kernel32.dll, respectivamente
    // para abrir o arquivo e ler seu conteúdo.
    CreateFileW_t pCreateFileW = (CreateFileW_t)GetProcAddress(hKernel32, "CreateFileW");
    ReadFile_t pReadFile = (ReadFile_t)GetProcAddress(hKernel32, "ReadFile");

    if (pCreateFileW == NULL || pReadFile == NULL) {
        // Se houve erro ao obter a função CreateFileW ou ReadFile, retorna false.
        wprintf(L"Erro ao obter endereços das funções.\n");
        FreeLibrary(hKernel32);
        return FALSE;
    }

    // Abre o arquivo para leitura.
    HANDLE hFile = pCreateFileW(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        // Se houve erro ao abrir o arquivo, retorna false.
        wprintf(L"Erro ao abrir o arquivo para leitura.\n");
        FreeLibrary(hKernel32);
        return FALSE;
    }

    // Número de bytes lidos do arquivo.
    DWORD bytesRead;

    // Buffer aonde os bytes lidos serão armazenados. O buffer pode conter até 1024 bytes.
    // Como obtém apenas o texto "Ola mundo!", vai sobrar a maioria do buffer, e isso
    // é intencional, para demonstrar a função do caracterer '\0' finalizando a string.
    char buffer[1024];

    // Lê os primeiros 1024 bytes do arquivo no buffer, ou, no caso, todo o conteúdo do 
    // arquivo, pois este tem menos de 1024 bytes.
    BOOL result = pReadFile(
        hFile,
        buffer,
        sizeof(buffer) - 1,
        &bytesRead,
        NULL
    );

    // Se não houve erro ao ler o arquivo, vai exibir o texto recuperado do mesmo na caixa de diálogo,
    // senão exibe uma mensagem de erro no prompt.

    if (result) {

        wprintf(L"Exibindo o conteúdo do arquivo no diálogo...\n");

        wchar_t wbuffer[sizeof(buffer)];

        // Copia buffer (do tipo char*) para wbuffer (do tipo wchar_t*). Como o texto termina com '\0', 
        // não será tratado o alerta do compilador sobre a possível falta deste caractere.
        mbstowcs_s(NULL, wbuffer, sizeof(wbuffer) / sizeof(wbuffer[0]), buffer, sizeof(buffer) - 1);

        // Exibe o conteúdo do arquivo no diálogo.
        showMessageBox(wbuffer);

    }
    else {

        wprintf(L"Erro ao ler o arquivo.\n");

    }

    CloseHandle(hFile);
    FreeLibrary(hKernel32);

    return result;

}


/// <summary>
/// Este método grava o array de BYTE chamado "data" passado como parâmetro no arquivo com o
/// path definido em fileName.
/// </summary>
/// <param name="filename">Path do arquivo a ser escrito o array de bytes.</param>
/// <param name="data">Array de bytes a ser gravado no arquivo.</param>
/// <param name="dataSize">Número de bytes do array a serem gravados no arquivo.</param>
/// <returns>True, se o arquivo foi gravado. False se houve erro ao gravar o arquivo.</returns>
BOOL writeFile(const wchar_t* filename, const BYTE* data, DWORD dataSize) {

    // Carrega kernel32.dll para a área de memória do programa.
    HMODULE hKernel32 = LoadLibraryEx(L"kernel32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

    if (hKernel32 == NULL) {
        // Se houve erro ao carregar kernel32.dll, retorna false.
        wprintf(L"Erro ao carregar kernel32.dll.\n");
        return FALSE;
    }

    // Obtém os endereços das funções CreateFileW e WriteFile de kernel32.dll, respectivamente para
    // abrir o arquivo e gravar dados nele.
    CreateFileW_t pCreateFileW = (CreateFileW_t)GetProcAddress(hKernel32, "CreateFileW");
    WriteFile_t pWriteFile = (WriteFile_t)GetProcAddress(hKernel32, "WriteFile");

    if (pCreateFileW == NULL || pWriteFile == NULL) {
        // Se houve erro ao obter a função CreateFileW ou WriteFile, retorna false.
        wprintf(L"Erro ao obter endereços das funções.\n");
        FreeLibrary(hKernel32);
        return FALSE;
    }

    // Chama CreateFile para abrir/criar o arquivo.
    HANDLE hFile = pCreateFileW(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        // Se houve erro ao abrir/criar o arquivo, retorna false.
        wprintf(L"Erro ao abrir o arquivo para escrita.\n");
        FreeLibrary(hKernel32);
        return FALSE;
    }

    // Número de bytes gravados no arquivo.
    DWORD bytesWritten;

    // Grava os bytes do vetor data no arquivo.
    BOOL result = pWriteFile(
        hFile,
        data,
        dataSize,
        &bytesWritten,
        NULL
    );

    CloseHandle(hFile);
    FreeLibrary(hKernel32);

    if (result) {
        wprintf(L"Arquivo gravado com sucesso.\n");
    }
    else {
        wprintf(L"Erro ao gravar o arquivo.\n");
    }

    return result;

}




/// <summary>
/// Ponto de entrada do programa. Ao iniciar, executa estes 3 passos:
/// 
/// 1. Cria o arquivo e escreve o texto "Ola mundo!" nele;
/// 
/// 2. Abre o arquivo e lê o texto;
/// 
/// 3. Exibe a caixa de diálogo com o texto recuperado do arquivo.
/// 
/// O nome do arquivo é "TestFile.txt" e ele está localizado no diretório raiz do
/// programa.
/// </summary>
/// <returns>Status de execução normal.</returns>
int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    const wchar_t* filename = L"TestFile.txt";

    BYTE data[] = { 'O', 'l', 'a', ' ', 'm', 'u', 'n', 'd', 'o', '!', '\0' };

    DWORD dataSize = sizeof(data);

    if (writeFile(filename, data, dataSize)) {

        readFile(filename);

    }

    wprintf(L"Pressiona qualquer tecla para sair...");

    getchar();

    return 0;

}