
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
struct Memory 
  {
  char *buffer;
  size_t size;
  };

// Função callback para escrever os dados da resposta no buffer
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) 
  {
  size_t realsize = size * nmemb;
  struct Memory *mem = (struct Memory *)userp;

  char *ptr = realloc(mem->buffer, mem->size + realsize + 1);
  if (ptr == NULL) 
    {
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
void hmac_sha256(const char *key, const char *data, char *output) 
  {
  unsigned char *result;
  unsigned int len = 32;

  result = HMAC(EVP_sha256(), key, strlen(key), (unsigned char *)data, strlen(data), NULL, NULL);

  for (unsigned int i = 0; i < len; i++) 
    {
    sprintf(output + (i * 2), "%02x", result[i]);
    }
  output[64] = 0;
  }

// Função de callback para capturar o tempo do servidor Binance
static size_t WriteTimeCallback(void *contents, size_t size, size_t nmemb, void *userp) 
  {
  size_t realsize = size * nmemb;
  long long *server_time = (long long *)userp;
  char *json_response = (char *)contents;

  // Extrair o timestamp do JSON retornado
  sscanf(json_response, "{\"serverTime\":%lld}", server_time);

  return realsize;
  }

// Função para obter o tempo do servidor Binance
long long get_server_time() 
  {
  CURL *curl;
  CURLcode res;
  long long server_time = 0;

  curl = curl_easy_init();

  if (curl) 
    {
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.binance.com/api/v3/time");

    // Configurar a funÃƒÆ’Ã‚Â§ÃƒÆ’Ã‚Â£o de callback para escrever a resposta
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteTimeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &server_time);

    // Fazer a requisiÃƒÆ’Ã‚Â§ÃƒÆ’Ã‚Â£o
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) 
	  {
      fprintf(stderr, "Erro ao acessar a API Binance para obter o tempo: %s\n", curl_easy_strerror(res));
      }

    curl_easy_cleanup(curl);
    }
  return server_time;
  }

// Função para imprimir o saldo e o ID da conta
void 
print_balances_and_account_id(const char *json_str) 
  {
  cJSON *json = cJSON_Parse(json_str);  // Parse do JSON
  if (json == NULL) 
    {
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
  if (balances != NULL && cJSON_IsArray(balances)) 
    {
    cJSON *balance = NULL;
    cJSON_ArrayForEach(balance, balances) 
	  {
      cJSON *asset = cJSON_GetObjectItemCaseSensitive(balance, "asset");
      cJSON *free = cJSON_GetObjectItemCaseSensitive(balance, "free");
      cJSON *locked = cJSON_GetObjectItemCaseSensitive(balance, "locked");

      if (asset && free && locked) 
        {
        // Verificar saldo
	    if (atof(free->valuestring) > 0 || atof(locked->valuestring) > 0)
	      {
	      //Mostrar saldo
	      printf("Moeda: %s | Free: %s | Locked: %s\n", asset->valuestring, free->valuestring, locked->valuestring);
	      }
        }
      }
    } else 
        {
        printf("Erro: Não foi possível encontrar o campo 'balances' no JSON.\n");
        }

    cJSON_Delete(json);  // Liberar memória alocada pelo cJSON
  }

//Função de ordem de compra
void 
place_limit_buy_order(const char *symbol, double price, double quantity) 
  {
   printf("Enviando ordem de compra para %f %s ao preço de %.8f USDT...\n", quantity, symbol, price);
    fflush(stdout); // Garante que a mensagem acima apareça imediatamente

    // --- DEBUG: Início ---
    //printf("--- DEBUG: Carregando chaves de API...\n");
    fflush(stdout);
    
    char *api_key = getenv("BINANCE_API_KEY");
    char *secret_key = getenv("BINANCE_SECRET_KEY");

    if (api_key == NULL || secret_key == NULL) {
        printf("Erro Crítico: Não foi possível encontrar as chaves de API para enviar a ordem.\n");
        return;
    }

    CURL *curl;
    CURLcode res;
    struct Memory response;
    response.buffer = malloc(1);
    response.size = 0;

    curl = curl_easy_init();
    if (curl) {
        char endpoint[] = "https://api.binance.com/api/v3/order";
        
        // --- DEBUG: Buscando timestamp ---
        //printf("--- DEBUG: Chamando get_server_time...\n");
        fflush(stdout);
        long long timestamp = get_server_time();
        //printf("--- DEBUG: get_server_time retornou. Timestamp: %lld\n", timestamp);
        fflush(stdout);

        char query[512];
        snprintf(query, sizeof(query), "symbol=%s&side=BUY&type=LIMIT&timeInForce=GTC&quantity=%.8f&price=%.8f&timestamp=%lld",
                 symbol, quantity, price, timestamp);

        // --- DEBUG: Gerando assinatura ---
        //printf("--- DEBUG: Gerando assinatura HMAC...\n");
        fflush(stdout);
        char signature[65];
        hmac_sha256(secret_key, query, signature);
        //printf("--- DEBUG: Assinatura gerada.\n");
        fflush(stdout);

        char url[1024];
        snprintf(url, sizeof(url), "%s?%s&signature=%s", endpoint, query, signature);

        struct curl_slist *headers = NULL;
        char api_key_header[256];
        snprintf(api_key_header, sizeof(api_key_header), "X-MBX-APIKEY: %s", api_key);
        headers = curl_slist_append(headers, api_key_header);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L); //Tamanho do corpo do POST = zero
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
	
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        // --- DEBUG: Ponto crítico ---
        //printf("--- DEBUG: EXECUTANDO a requisição de rede (curl_easy_perform)...\n");
        fflush(stdout);
        res = curl_easy_perform(curl);
        // Se o programa travar, ele não imprimirá a próxima linha
        // printf("--- DEBUG: Requisição de rede FINALIZADA.\n");
        fflush(stdout);

        if (res != CURLE_OK) {
            fprintf(stderr, "Erro ao enviar ordem de compra: %s\n", curl_easy_strerror(res));
        } else {
            printf("Resposta da ordem de compra: %s\n", response.buffer);
        }

        free(response.buffer);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
  }

//Função de ordem de venda
void place_limit_sell_order(const char *symbol, double price, double quantity) 
{
    printf("Enviando ordem de venda para %f %s ao preço de %.8f...\n", quantity, symbol, price);

    char *api_key = getenv("BINANCE_API_KEY");
    char *secret_key = getenv("BINANCE_SECRET_KEY");

    if (api_key == NULL || secret_key == NULL) {
        printf("Erro Crítico: Chaves de API não encontradas.\n");
        return;
    }

    CURL *curl;
    CURLcode res;
    struct Memory response;
    response.buffer = malloc(1);
    response.size = 0;

    curl = curl_easy_init();
    if (curl) {
        char endpoint[] = "https://api.binance.com/api/v3/order";
        long long timestamp = get_server_time();

        char query[512];
        // A ÚNICA GRANDE MUDANÇA ESTÁ AQUI: side=SELL
        snprintf(query, sizeof(query), "symbol=%s&side=SELL&type=LIMIT&timeInForce=GTC&quantity=%.8f&price=%.8f&timestamp=%lld",
                 symbol, quantity, price, timestamp);

        char signature[65];
        hmac_sha256(secret_key, query, signature);

        char url[1024];
        snprintf(url, sizeof(url), "%s?%s&signature=%s", endpoint, query, signature);

        struct curl_slist *headers = NULL;
        char api_key_header[256];
        snprintf(api_key_header, sizeof(api_key_header), "X-MBX-APIKEY: %s", api_key);
        headers = curl_slist_append(headers, api_key_header);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "Erro ao enviar ordem de venda: %s\n", curl_easy_strerror(res));
        } else {
            printf("Resposta da ordem de venda: %s\n", response.buffer);
        }

        free(response.buffer);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

int
main() 
  {
  curl_global_init(CURL_GLOBAL_DEFAULT);
    int opcao;
    char trade_symbol[20] = "";
    double target_price = 0.0;
    double quantity = 0.0;
    int trade_status = 0; // 0=Inativo, 1=Aguardando Compra, 2=Aguardando Venda

    printf(" _____  ____      _     ____   _____   ____   / _ \\ |_   _|   ____   |_ _| | \\ | |   / \\   | \\ | |  / ___| | ____|\n");
    printf("|_   _| |  _ \\    / \\   |  _ \\ | ____|  |  _ \\  | | | |  | |    |  _ \\   | |  |  \\| |  / _ \\  |  \\| | | |    |  _|  \n");
    printf("  | |   | |_) |  / _ \\  | | | | |  _|    | |_) | | |_| |  | |    | |_) |  | |  | |\\  | / ___ \\ | |\\  | | |___ | |___ \n");
    printf("  |_|   |_| _\\ /_/   _\\ |____/  |_____|  |____/   \\___/   |_|    |____/  |___| |_| \\_|/_/   \\_\\|_| \\_|  \\____| |_____| ~ By Celso Jr\n");

    // Loop principal do menu
    while (1) 
    {
        printf("\nPainel de Controle: \n");
        printf("\n1 - Saldo Disponivel\n");
        printf("2 - Trade\n");
        printf("3 - Sair\n");
        printf("\nEscolha uma opcao: ");
        scanf(" %d", &opcao); // Espaço antes de %d para limpar buffer

        // Lógica para a Opção 1: Ver Saldo
        if (opcao == 1) 
        {
            printf("\nConectando ao servidor da Binance...\n");
            // ... (SEU CÓDIGO PARA A OPÇÃO 1 VAI AQUI, SEM MUDANÇAS) ...
            // Eu o omiti para focar na correção, mas você deve mantê-lo.
        } 
        // Lógica para a Opção 2: Trade
        else if (opcao == 2)
        {
            if (trade_status != 0) {
                printf("\n--- ERRO: UMA TRADE JÁ ESTÁ EM ANDAMENTO ---\n");
            } else {
                int trade_choice = 0;
                printf("\n--- CONFIGURAR NOVA ORDEM ---\n");
                printf("1 - Configurar Ordem de Compra\n");
                printf("2 - Configurar Ordem de Venda\n");
                printf("Escolha uma opção: ");
                scanf(" %d", &trade_choice);

                // --- LÓGICA DE COMPRA ---
                if (trade_choice == 1)
                {
                    printf("\n--- NOVA ORDEM DE COMPRA ---\n");
                    printf("Digite o par de moedas (ex: BTCUSDT): ");
                    scanf("%s", trade_symbol);
                    printf("Digite o preço de compra desejado: ");
                    scanf("%lf", &target_price);
                    printf("Digite a quantidade que deseja comprar: ");
                    scanf("%lf", &quantity);

                    trade_status = 1;
                    printf("\nOrdem configurada! Iniciando monitoramento dedicado...\n");
                    
                    // Loop de monitoramento DEDICADO para a compra
                    while (trade_status == 1) {
                        // COLE AQUI o seu código de monitoramento de compra que já funciona
                    }
                    printf("\nMonitoramento finalizado. Retornando ao menu principal.\n");
                } 
                // --- LÓGICA DE VENDA ---
                else if (trade_choice == 2)
                {
                    printf("\n--- NOVA ORDEM DE VENDA ---\n");
                    printf("Digite o par de moedas (ex: BTCUSDT): ");
                    scanf("%s", trade_symbol);
                    printf("Digite o preço de venda desejado: ");
                    scanf("%lf", &target_price);
                    printf("Digite a quantidade que deseja vender: ");
                    scanf("%lf", &quantity);

                    trade_status = 2;
                    printf("\nOrdem de venda configurada! Iniciando monitoramento dedicado...\n");
                    
                    // Loop de monitoramento DEDICADO para a venda
                    while (trade_status == 2) {
                        // COLE AQUI o código de monitoramento de VENDA que te passei antes
                    }
                    printf("\nMonitoramento finalizado. Retornando ao menu principal.\n");
                }
                else 
                {
                    printf("Opção de trade inválida.\n");
                }
            }
        }
        // Lógica para a Opção 3: Sair
        else if (opcao == 3) 
        {
            break; // Sai do loop while(1)
        } 
        else 
        {
            printf("\nOpcao invalida. Tente novamente.\n");
        }
    } // Fim do while(1)

    curl_global_cleanup();
    return 0;
  }
