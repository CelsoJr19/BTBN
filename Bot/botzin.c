
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <time.h>
#include <cjson/cJSON.h>  // Bibliotecas cJSON
#include <unistd.h>

// Estrutura para armazenar a resposta da API
struct Memory {
    char *buffer;
    size_t size;
};

// Função callback para escrever os dados da resposta no buffer
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    char *ptr = realloc(mem->buffer, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Erro: não foi possível alocar memória para a resposta.\n");
        return 0;
    }

    mem->buffer = ptr;
    memcpy(&(mem->buffer[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->buffer[mem->size] = 0;

    return realsize;
}

// Função para calcular o HMAC SHA256
void hmac_sha256(const char *key, const char *data, char *output) {
    unsigned char *result;
    unsigned int len = 32;

    result = HMAC(EVP_sha256(), key, strlen(key), (unsigned char *)data, strlen(data), NULL, NULL);

    for (unsigned int i = 0; i < len; i++) {
        sprintf(output + (i * 2), "%02x", result[i]);
    }
    output[64] = 0;
}

// Função de callback para capturar o tempo do servidor Binance
static size_t WriteTimeCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    long long *server_time = (long long *)userp;
    char *json_response = (char *)contents;

    // Extrair o timestamp do JSON retornado
    sscanf(json_response, "{\"serverTime\":%lld}", server_time);

    return realsize;
}

// Função para obter o tempo do servidor Binance
long long get_server_time() {
    CURL *curl;
    CURLcode res;
    long long server_time = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.binance.com/api/v3/time");

        // Configurar a funÃƒÆ’Ã‚Â§ÃƒÆ’Ã‚Â£o de callback para escrever a resposta
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteTimeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &server_time);

        // Fazer a requisiÃƒÆ’Ã‚Â§ÃƒÆ’Ã‚Â£o
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Erro ao acessar a API Binance para obter o tempo: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return server_time;
}

// Função para imprimir o saldo e o ID da conta
void print_balances_and_account_id(const char *json_str) {
    cJSON *json = cJSON_Parse(json_str);  // Parse do JSON
    if (json == NULL) {
        printf("Erro ao parsear o JSON.\n");
        return;
    }
    
    //Encontrar ID
    cJSON *uid = cJSON_GetObjectItemCaseSensitive(json, "uid");
    if (uid != NULL && cJSON_IsNumber(uid))
      {
      long long uid_value = uid->valuedouble;
      printf("\nID da conta: %lld\n", uid_value);
      } 
    else
      {
      printf("Erro: ID não encontrado no JSON.\n");
      }

    // Encontrar o array de saldos
    cJSON *balances = cJSON_GetObjectItemCaseSensitive(json, "balances");
    if (balances != NULL && cJSON_IsArray(balances)) {
        cJSON *balance = NULL;
        cJSON_ArrayForEach(balance, balances) {
            cJSON *asset = cJSON_GetObjectItemCaseSensitive(balance, "asset");
            cJSON *free = cJSON_GetObjectItemCaseSensitive(balance, "free");
            cJSON *locked = cJSON_GetObjectItemCaseSensitive(balance, "locked");

            if (asset && free && locked) {
                // Verificar saldo
		if (atof(free->valuestring) > 0 || atof(locked->valuestring) > 0)
		  {
	          //Mostrar saldo
		  printf("Moeda: %s | Free: %s | Locked: %s\n", asset->valuestring, free->valuestring, locked->valuestring);
	          }
                }
            }
        } else {
           printf("Erro: Não foi possível encontrar o campo 'balances' no JSON.\n");
    }

    cJSON_Delete(json);  // Liberar memória alocada pelo cJSON
  }


int
main() 
  {
  int opcao;
  char trade_symbol[20] = ""; // Ex: "BTCUSDT"
  double target_price = 0.0;
  double quantity = 0.0;
  // 0 = Inativo, 1 = Aguardando Compra, 2 = Aguardando Venda
  int trade_status = 0; 

  printf(" _____   ____       _      ____    _____     ____     ___    _____     ____    ___   _   _      _      _   _    ____   _____ \n");
  printf("|_   _| |  _ \\     / \\    |  _ \\  | ____|   | __ )   / _ \\  |_   _|   | __ )  |_ _| | \\ | |    / \\    | \\ | |  / ___| | ____|\n");
  printf("  | |   | |_) |   / _ \\   | | | | |  _|     |  _ \\  | | | |   | |     |  _ \\   | |  |  \\| |   / _ \\   |  \\| | | |     |  _|  \n");
  printf("  | |   |  _ <   / ___ \\  | |_| | | |___    | |_) | | |_| |   | |     | |_) |  | |  | |\\  |  / ___ \\  | |\\  | | |___  | |___ \n");
  printf("  |_|   |_|  _\\ /_/    _\\ |____/  |_____|   |____/   \\___/    |_|     |____/  |___| |_| \\_| /_/   \\_\\ |_| \\_|  \\____| |_____| ~ By Celso Jr\n");
 
  while (1) 
    {
	if (trade_status == 1) 
	  { // Se esta aguardando uma compra...
        // ===================================================================
        // AQUI VAI ENTRAR O CÓDIGO QUE BUSCA O PREÇO E COMPARA COM O ALVO
        // Por enquanto, vamos apenas imprimir uma mensagem para ver que funciona
        // ===================================================================
      printf("\n[MONITORANDO] Verificando preço de %s... (Alvo: %.2f)\n", trade_symbol, target_price);
      }
		
    printf("\nPainel de Controle: \n");
    printf("\n1 - Saldo Disponivel\n");
    printf("2 - Trade\n");
    printf("3 - Sair\n");
    printf("\nEscolha uma opcao: ");
    printf("\n");
    scanf("%d", &opcao);
        
    if (opcao == 1) 
      {
      printf("\nConectando ao servidor da Binance...\n");
        // Carregar variÃƒÆ’Ã‚Â¡veis de ambiente
    char *api_key = getenv("BINANCE_API_KEY");
    char *secret_key = getenv("BINANCE_SECRET_KEY");

    if (api_key == NULL || secret_key == NULL) {
        printf("Erro: API Key ou Secret Key nÃƒÆ’Ã‚Â£o encontradas. Verifique o arquivo config.env.\n");
        return 1;
    }

    CURL *curl;
    CURLcode res;

    struct Memory response;
    response.buffer = malloc(1); // Inicializa o buffer
    response.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        char endpoint[] = "https://api.binance.com/api/v3/account";
        long recvWindow = 60000;

        // Obter o tempo do servidor da Binance
        long long timestamp = get_server_time(); // Usando o tempo do servidor

        // Criar a string de consulta
        char query[256];
        snprintf(query, sizeof(query), "timestamp=%lld&recvWindow=%ld", timestamp, recvWindow);

        // Gerar a assinatura
        char signature[65];
        hmac_sha256(secret_key, query, signature);

        // Construir a URL final com assinatura
        char url[512];
        snprintf(url, sizeof(url), "%s?%s&signature=%s", endpoint, query, signature);

        // Configurar os cabeÃƒÆ’Ã‚Â§alhos
        struct curl_slist *headers = NULL;
        char api_key_header[128];
        snprintf(api_key_header, sizeof(api_key_header), "X-MBX-APIKEY: %s", api_key);
        headers = curl_slist_append(headers, api_key_header);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        // Fazer a requisão
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "Erro ao acessar a API Binance: %s\n", curl_easy_strerror(res));
        } else {
            // Chamar a funÃƒÆ’Ã‚Â§ÃƒÆ’Ã‚Â£o para imprimir o saldo de BTC e o ID da conta
            print_balances_and_account_id(response.buffer);
        }

        // Liberar recursos
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    free(response.buffer);
    curl_global_cleanup();
    
        } 
	
	else if (opcao == 2)
        {
            if (trade_status != 0) {
                // Se uma trade já está ativa, apenas mostra o status
                printf("\n--- STATUS DA TRADE ATIVA ---\n");
                printf("Par: %s\n", trade_symbol);
                printf("Quantidade: %f\n", quantity);
                if (trade_status == 1) {
                    printf("Ação: AGUARDANDO PREÇO DE COMPRA ATINGIR %.2f\n", target_price);
                } else if (trade_status == 2) {
                    printf("Ação: AGUARDANDO PREÇO DE VENDA ATINGIR %.2f\n", target_price);
                }
                printf("---------------------------\n");
            } else {
                // Se nenhuma trade está ativa, configura uma nova
                printf("\n--- CONFIGURAR NOVA ORDEM DE COMPRA ---\n");

                printf("Digite o par de moedas (ex: BTCUSDT): ");
                scanf("%s", trade_symbol);

                printf("Digite o preço de compra desejado (em USDT): ");
                scanf("%lf", &target_price);

                printf("Digite a quantidade de %s que deseja comprar: ", trade_symbol);
                scanf("%lf", &quantity);

                trade_status = 1; // Define o estado para "Aguardando Compra"

                printf("\nOrdem configurada! O bot começará a monitorar o preço para comprar %s a %.2f USDT.\n", trade_symbol, target_price);
            }
        }

		else if (opcao == 3) 
          	{
          	return 0;
          	}   
		else 
	  	{
          	printf("\nOpcao invalida. Tente novamente.\n");
          	}
    	}

  	return 0;
  	}
