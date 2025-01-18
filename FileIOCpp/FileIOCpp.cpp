#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>

/*
    Este programa faz exatamente a mesma coisa que o programa em C faz, usando chamadas às API do
    Windows, porém este utiliza os recursos padrão da linguagem C++, que "por debaixo do capô", também
    fazem uso de chamadas à API do Windows, porém abstraídas em recursos da própria linguagem como nas
    classes de stream std::wofstream e std::wifstream.
*/

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Nome do arquivo
    const std::wstring filename = L"TestFile.txt";

    // Abre o arquivo para escrita.
    std::wofstream outFile(filename);

    if (!outFile) {
        std::wcerr << L"Erro ao criar o arquivo para escrita." << std::endl;
        return 1;
    }
    else {
        std::wcout << L"Arquivo gravado com sucesso." << std::endl;
    }

    // Escreve o texto no arquivo.
    outFile << L"Ola mundo!";

    outFile.close();

    // Abre o arquivo para leitura
    std::wifstream inFile(filename);

    if (!inFile) {
        std::wcerr << L"Erro ao abrir o arquivo para leitura." << std::endl;
        return 1;
    }

    // Obtém o texto gravado no arquivo.
    std::wstring textRead((std::istreambuf_iterator<wchar_t>(inFile)), std::istreambuf_iterator<wchar_t>());

    inFile.close();

    std::wcout << L"Exibindo o conteúdo do arquivo no diálogo..." << std::endl;

    // Exibe o conteúdo do arquivo em um diálogo
    int btn = MessageBoxW(NULL, textRead.c_str(), L"Texto do arquivo", MB_OKCANCEL | MB_ICONINFORMATION);

    if (btn == 1) {
        std::wcout << L"Você clicou no botão OK." << std::endl;
    }
    else {
        std::wcout << L"Você clicou no botão Cancelar." << std::endl;
    }

    std::string any;
    std::wcout << "Pressione qualquer tecla para sair...";
    std::getline(std::cin, any);

    return 0;

}