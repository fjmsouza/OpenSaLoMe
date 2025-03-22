import os
import sys
import subprocess
Import("env")

# Defina os valores de início e tamanho da região SPIFFS (ajuste conforme sua tabela de partições)
SPIFFS_START = "0x410000"  # Exemplo: endereço de início do SPIFFS
SPIFFS_SIZE  = "0x200000"   # Exemplo: tamanho do SPIFFS

def get_esptool_path():
    # Tenta obter o caminho definido na board config, se houver
    esptool = env.BoardConfig().get("build.esptool", "esptool.py")
    # Se o caminho não for absoluto, tente localizá-lo na pasta do pacote tool-esptoolpy
    if not os.path.isabs(esptool):
        tool_dir = env.PioPlatform().get_package_dir("tool-esptoolpy")
        candidate = os.path.join(tool_dir, esptool)
        if os.path.isfile(candidate):
            return candidate
    return esptool

def erase_spiffs_action(target, source, env):
    port = env.GetProjectOption("upload_port")
    if not port:
        port = env.subst("$UPLOAD_PORT")
    esptool_path = get_esptool_path()
    
    # Constrói o comando para execução
    command = [
        sys.executable,
        esptool_path,
        "--chip", "esp32s3",
        "--port", port,
        "erase_region",
        SPIFFS_START,
        SPIFFS_SIZE
    ]
    print("Apagando a região LittleFS na flash com o comando:")
    print(" ".join(command))
    subprocess.check_call(command)

env.AddCustomTarget(
    name="erase_storage",
    dependencies=[],
    actions=[erase_spiffs_action],
    title="Erase storage",
    description="Apaga somente a região LittleFS da flash"
)
