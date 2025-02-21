#include "Storage.h"

void Storage::setup() {
  if (!LittleFS.begin()) {
    Serial.println("Erro ao montar o sistema de arquivos LittleFS!");
    return;
  }
  Serial.println("Montou sistema de arquivos LittleFS!");
  return;
}

void Storage::writeString(const char *caminho, String dado_escrever) {
  const char *nomeArquivo = caminho;

  // Se o arquivo não existe, cria e escreve
  if (!LittleFS.exists(nomeArquivo)) {
    Serial.println("Arquivo não encontrado. Criando e escrevendo dado...");
    
    // Remove o format() para evitar apagar toda a memória
    // LittleFS.format(); // NÃO CHAME FORMAT AQUI!

    File file = LittleFS.open(nomeArquivo, FILE_WRITE);
    if (!file) {
      Serial.println("Falha ao abrir o arquivo para escrita!");
      return;
    } 
    file.print(dado_escrever);  // Usa print() para evitar nova linha automática
    file.close();
    Serial.println("Arquivo criado e escrito com sucesso!");
  } else {
    Serial.println("Arquivo já existe. Escrevendo novo dado...");
    
    // Abre o arquivo e sobrescreve o conteúdo
    File file = LittleFS.open(nomeArquivo, FILE_WRITE);
    if (!file) {
      Serial.println("Erro ao abrir o arquivo para escrita!");
      return;
    }
    file.print(dado_escrever);
    file.close();
    Serial.println("Dado sobrescrito com sucesso!");
  }

  // Agora, abre o arquivo para leitura e verifica se foi salvo corretamente
  File file = LittleFS.open(nomeArquivo, FILE_READ);
  if (!file) {
    Serial.println("Erro ao abrir o arquivo para leitura!");
    return;
  }
  
  String data = "";
  Serial.println("Conteúdo do arquivo:");
  while (file.available()) {
    data += (char)file.read();
  }
  Serial.println(data);  // Agora deve imprimir corretamente o conteúdo salvo
  file.close();
}


String Storage::readString(const char *caminho) {
  const char *nomeArquivo = caminho;

  File file;
  // Verifica se o arquivo já existe
  if (!LittleFS.exists(nomeArquivo)) {
    Serial.println("Arquivo não encontrado!");
    return "NOT_SAVED";
  }

  // Agora, abre o arquivo para leitura
  file = LittleFS.open(nomeArquivo, FILE_READ);
  if (!file) {
    Serial.println("Erro ao abrir o arquivo para leitura!");
    return "NOT_SAVED";
  }

  Serial.println("Conteúdo do arquivo:");
  String data = "";  // Inicializa a string vazia
  while (file.available()) {
    data += (char)file.read();  // Concatena os caracteres lidos
  }
  Serial.println(data);
  file.close();
  return data;
}

bool Storage::fileExists(const char *caminho) {
  const char *nomeArquivo = caminho;

  File file;
  // Verifica se o arquivo já existe
  if (!LittleFS.exists(nomeArquivo)) {
    Serial.println("Arquivo não encontrado!");
    return false;
  }

  return true;
}

void Storage::createFile(const char *caminho) {
  const char *nomeArquivo = caminho;

  Serial.println("Criando arquivo...");

  // cria arquivo
  File file;
  file = LittleFS.open(nomeArquivo, FILE_WRITE);
  if (!file) {
    Serial.println("Falha ao abrir o arquivo para escrita!");
  } else {
    file.close();
    Serial.println("Arquivo criado!");
    return;
  }
}
