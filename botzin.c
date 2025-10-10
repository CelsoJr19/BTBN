
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <time.h>
#include "cJSON.h"  // Bibliotecas cJSON
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
  
  printf(" _____   ____       _      ____    _____     ____     ___    _____     ____    ___   _   _      _      _   _    ____   _____ \n");
  printf("|_   _| |  _ \\     / \\    |  _ \\  | ____|   | __ )   / _ \\  |_   _|   | __ )  |_ _| | \\ | |    / \\    | \\ | |  / ___| | ____|\n");
  printf("  | |   | |_) |   / _ \\   | | | | |  _|     |  _ \\  | | | |   | |     |  _ \\   | |  |  \\| |   / _ \\   |  \\| | | |     |  _|  \n");
  printf("  | |   |  _ <   / ___ \\  | |_| | | |___    | |_) | | |_| |   | |     | |_) |  | |  | |\\  |  / ___ \\  | |\\  | | |___  | |___ \n");
  printf("  |_|   |_|  _\\ /_/    _\\ |____/  |_____|   |____/   \\___/    |_|     |____/  |___| |_| \\_| /_/   \\_\\ |_| \\_|  \\____| |_____| ~ By Celso Jr\n");
 
  while (1) 
    {
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

        // Fazer a requisiÃƒÆ’Ã‚Â§ÃƒÆ’Ã‚Â£o
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
	printf("\nConectando ao servidor da Binance...\n");

    // Carregar variáveis de ambiente
    char *api_key = getenv("BINANCE_API_KEY");
    char *secret_key = getenv("BINANCE_SECRET_KEY");

    if (api_key == NULL || secret_key == NULL) {
        printf("Erro: API Key ou Secret Key não encontradas. Verifique o arquivo config.env.\n");
        return 1;
    }

    CURL *curl;
    CURLcode res;
    struct Memory response;

    response.buffer = malloc(1);
    response.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    double preco_compra, preco_venda, quantidade_btc; 

    // Solicitar ao usuário os preços desejados e a quantidade de BRL a investir
    printf("Digite o preço de compra desejado (em USD): ");
    scanf("%lf", &preco_compra);

    printf("Digite o preço de venda desejado (em USD): ");
    scanf("%lf", &preco_venda);

    printf("Digite a quantidade de BTC que deseja comprar e vender: ");
    scanf("%lf", &quantidade_btc);

    printf("\nMonitorando os preços de BTC/USDT para executar a trade...\n");

    if (curl) {
        while (1) {
            // Endpoint para obter o preço atual do BTC/USDT
            char endpoint[] = "https://api.binance.com/api/v3/ticker/price?symbol=BTCUSDT";

            curl_easy_setopt(curl, CURLOPT_URL, endpoint);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                fprintf(stderr, "Erro ao acessar a API Binance: %s\n", curl_easy_strerror(res));
                break;
            }

            cJSON *json = cJSON_Parse(response.buffer);
            if (json == NULL) {
                printf("Erro ao parsear o JSON.\n");
                break;
            }

            cJSON *price_item = cJSON_GetObjectItemCaseSensitive(json, "price");
            if (price_item && cJSON_IsString(price_item)) {
                double preco_atual = atof(price_item->valuestring);
                printf("Preço atual do BTC/USDT: %.2f USD\n", preco_atual);

                // Se o preço atingir o valor de compra desejado, realiza a compra
                if (preco_atual <= preco_compra) {
                    printf("\nExecutando compra de BTC...\n");

                    // Realizar a ordem de compra via API
                    char query[1024];
                    long long timestamp = get_server_time();
                    snprintf(query, sizeof(query), "symbol=BTCUSDT&side=BUY&type=LIMIT&price=%.2f&quantity=%.8f&timeInForce=GTC&timestamp=%lld", preco_atual, quantidade_btc, timestamp);

                    char signature[65];
                    hmac_sha256(secret_key, query, signature);

                    char url[512];
                    snprintf(url, sizeof(url), "https://api.binance.com/api/v3/order?%s&signature=%s", query, signature);

                    struct curl_slist *headers = NULL;
                    char api_key_header[128];
                    snprintf(api_key_header, sizeof(api_key_header), "X-MBX-APIKEY: %s", api_key);
                    headers = curl_slist_append(headers, api_key_header);

                    curl_easy_setopt(curl, CURLOPT_URL, url);
                    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                    curl_easy_setopt(curl, CURLOPT_POST, 1);

                    res = curl_easy_perform(curl);

		    long http_code;
		    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		    printf("Status da resposta: %ld\n", http_code);

                    if (res != CURLE_OK) {
                        fprintf(stderr, "Erro ao realizar compra: %s\n", curl_easy_strerror(res));
                    } else {
                        printf("Compra realizada com sucesso!\n");
                        // Se o preço atingir o valor de venda desejado, realiza a venda
                		if (preco_atual >= preco_venda) 
						  {
                    		printf("\nExecutando venda de BTC...\n");

                    		// Realizar a ordem de venda via API
                    		char query[1024];
                    		long long timestamp = get_server_time();
                    		snprintf(query, sizeof(query), "symbol=BTCUSDT&side=SELL&type=LIMIT&price=%.2f&quantity=%.8f&timeInForce=GTC&timestamp=%lld", preco_atual, quantidade_btc, timestamp);

                    		char signature[65];
                    		hmac_sha256(secret_key, query, signature);

                    		char url[512];
                    		snprintf(url, sizeof(url), "https://api.binance.com/api/v3/order?%s&signature=%s", query, signature);

                    		struct curl_slist *headers = NULL;
                    		char api_key_header[128];
                    		snprintf(api_key_header, sizeof(api_key_header), "X-MBX-APIKEY: %s", api_key);
                    		headers = curl_slist_append(headers, api_key_header);

                    		curl_easy_setopt(curl, CURLOPT_URL, url);
                    		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                    		curl_easy_setopt(curl, CURLOPT_POST, 1);

                    		res = curl_easy_perform(curl);
                    		if (res != CURLE_OK) {
                        		fprintf(stderr, "Erro ao realizar venda: %s\n", curl_easy_strerror(res));
                    		} else {
                        		printf("Venda realizada com sucesso!\n");
                    		}
                		  }
                    }

                    break;
                }
            }

            cJSON_Delete(json);
            free(response.buffer);
            response.buffer = malloc(1);
            response.size = 0;

            // Esperar antes de verificar o preço novamente
            sleep(5);
        }

        curl_easy_cleanup(curl);
    }

    free(response.buffer);
    curl_global_cleanup();
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
