OBS: Usuarios de Windows que não conseguirem baixar o arquivo zip, favor usar o FIREFOX, pois o CHROME marca como virus!
Se ainda assim não permitir a extração, clique com o botão direito no arquivo zip e selecione o campo desbloquear na parte inferior de "Geral"
SE AINDA ASSIM não permitir, vá em "segurança do windows" - "Proteção contra virus e ameaças" - "Histórico de proteção" - "Ações" e selecione "Restaurar".

Para usar o BotTrade com sua conta Binance va até:
https://www.binance.com/en/my/settings/api-management
e crie uma chave de API HMAC.

Recomendações:
1. Cada conta pode criar até 30 chaves de API.
2. Não divulgue sua Chave de API, Chave Secreta (HMAC) ou Chave Privada (Ed25519, RSA) a ninguém para evitar perdas de ativos. Você deve tratar sua Chave de API e sua Chave Secreta (HMAC) ou Chave Privada (Ed25519, RSA) como suas senhas.
3. É recomendável restringir o acesso apenas a IPs confiáveis ​​para aumentar a segurança da sua conta.
4. Você não poderá criar uma chave de API se o KYC não for concluído.

Restrições de API:
- Habilite a Leitura
- Habilitae a negociação à vista e de margem
- Restrinja a IPs confiaveis

Obs: Caso opte por restringir os IPs, é necessario informar o IP a cada mudança de rede Wi-Fi que faça em sua maquina!
     Use _curl ifconfig.me_ para descobrir o IP atual.

Insira sua BINANCE_API_KEY e sua BINANCE_SECRET_KEY no arquivo "config.env"
Obs: O arquivo _"config.env"_ deve estar na mesma pasta que o _"botzin.exe"_ 

Obs WINDOWS: Aos usuarios de windows, o arquivo a ser alterado é o config.bat

Obs: Talves seja necessario digitar _source config.env_ no terminal para carregar as informações 

compile o código usando _gcc botzin.c -o botzin -lcurl -lcrypto -lcJSON_
