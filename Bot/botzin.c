#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <time.h>
#include <cjson/cJSON.h>
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

  if (api_key == NULL || secret_key == NULL)
    {
    printf("Erro Crítico: Não foi possível encontrar as chaves de API para enviar a ordem.\n");
    return;
    }

  CURL *curl;
  CURLcode res;
  struct Memory response;
  response.buffer = malloc(1);
  response.size = 0;

  curl = curl_easy_init();
  if (curl) 
    {
    char endpoint[] = "https://api.binance.com/api/v3/order";
        
    // --- DEBUG: Buscando timestamp ---
    //printf("--- DEBUG: Chamando get_server_time...\n");
    fflush(stdout);
    long long timestamp = get_server_time();
    //printf("--- DEBUG: get_server_time retornou. Timestamp: %lld\n", timestamp);
    fflush(stdout);

    char query[512];
    snprintf(query, sizeof(query), "symbol=%s&side=BUY&type=LIMIT&timeInForce=GTC&quantity=%.8f&price=%.8f&timestamp=%lld", symbol, quantity, price, timestamp);

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

    if (res != CURLE_OK) 
	  {
      fprintf(stderr, "Erro ao enviar ordem de compra: %s\n", curl_easy_strerror(res));
      } 
	else 
	  {
      printf("Resposta da ordem de compra: %s\n", response.buffer);
      }

    free(response.buffer);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    }
  }

//Função de ordem de venda
void 
place_limit_sell_order(const char *symbol, double price, double quantity) 
  {
  printf("Enviando ordem de venda para %f %s ao preço de %.8f...\n", quantity, symbol, price);

  char *api_key = getenv("BINANCE_API_KEY");
  char *secret_key = getenv("BINANCE_SECRET_KEY");

  if (api_key == NULL || secret_key == NULL) 
    {
    printf("Erro Crítico: Chaves de API não encontradas.\n");
    return;
    }

  CURL *curl;
  CURLcode res;
  struct Memory response;
  response.buffer = malloc(1);
  response.size = 0;

  curl = curl_easy_init();
  if (curl) 
    {
    char endpoint[] = "https://api.binance.com/api/v3/order";
    long long timestamp = get_server_time();

    char query[512];
    snprintf(query, sizeof(query), "symbol=%s&side=SELL&type=LIMIT&timeInForce=GTC&quantity=%.8f&price=%.8f&timestamp=%lld", symbol, quantity, price, timestamp);

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

    if (res != CURLE_OK) 
	  {
      fprintf(stderr, "Erro ao enviar ordem de venda: %s\n", curl_easy_strerror(res));
      } 
	else 
	  {
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
  double buy_target_price = 0.0;  // Preço alvo para COMPRAR no loop
  double sell_target_price = 0.0; // Preço alvo para VENDER no loop
  int trade_status = 0;
  // 0 = Inativo
  // 1 = Monitorando COMPRA ÚNICA
  // 2 = Monitorando VENDA ÚNICA
  // 3 = Loop: AGUARDANDO COMPRA
  // 4 = Loop: AGUARDANDO VENDA

  printf(" _____   ____       _      ____    _____     ____     ___    _____     ____    ___   _   _      _      _   _    ____   _____ \n");
  printf("|_   _| |  _ \\     / \\    |  _ \\  | ____|   | __ )   / _ \\  |_   _|   | __ )  |_ _| | \\ | |    / \\    | \\ | |  / ___| | ____|\n");
  printf("  | |   | |_) |   / _ \\   | | | | |  _|     |  _ \\  | | | |   | |     |  _ \\   | |  |  \\| |   / _ \\   |  \\| | | |     |  _|  \n");
  printf("  | |   |  _ <   / ___ \\  | |_| | | |___    | |_) | | |_| |   | |     | |_) |  | |  | |\\  |  / ___ \\  | |\\  | | |___  | |___ \n");
  printf("  |_|   |_|  _\\ /_/    _\\ |____/  |_____|   |____/   \\___/    |_|     |____/  |___| |_| \\_| /_/   \\_\\ |_| \\_|  \\____| |_____| ~ By Celso Jr\n");

  // Loop principal do menu
  while (1) 
    {
	if (trade_status == 1) 
        {
            printf("\n[MONITORANDO COMPRA ÚNICA] Alvo para %s: %.8f\n", trade_symbol, buy_target_price);
            
            char price_endpoint_url[256];
            snprintf(price_endpoint_url, sizeof(price_endpoint_url), "https://api.binance.com/api/v3/ticker/price?symbol=%s", trade_symbol);

            CURL *curl_price;
            CURLcode res_price;
            struct Memory price_response;
            price_response.buffer = malloc(1);
            price_response.size = 0;

            curl_price = curl_easy_init();
            if (curl_price) {
                curl_easy_setopt(curl_price, CURLOPT_CAINFO, "cacert.pem"); // Certificado SSL
                curl_easy_setopt(curl_price, CURLOPT_URL, price_endpoint_url);
                curl_easy_setopt(curl_price, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
                curl_easy_setopt(curl_price, CURLOPT_WRITEDATA, (void *)&price_response);
                res_price = curl_easy_perform(curl_price);

                if (res_price == CURLE_OK) {
                    cJSON *json = cJSON_Parse(price_response.buffer);
                    if (json != NULL) {
                        cJSON *price_item = cJSON_GetObjectItemCaseSensitive(json, "price");
                        if (cJSON_IsString(price_item) && (price_item->valuestring != NULL)) {
                            double current_price = atof(price_item->valuestring);
                            printf("Preço atual: %.8f\n", current_price);

                            if (current_price <= buy_target_price) {
                                printf("\n!!! ALVO DE COMPRA ÚNICA ATINGIDO !!!\n");
                                place_limit_buy_order(trade_symbol, buy_target_price, quantity);
                                trade_status = 0; 
                                strcpy(trade_symbol, "");
                            }
                        } else {
                             printf("ERRO: Não foi possível ler a chave 'price'. Verifique o par de moedas.\n");
                        }
                        cJSON_Delete(json);
                    }
                } else {
                    fprintf(stderr, "Erro ao buscar preço: %s\n", curl_easy_strerror(res_price));
                }
                curl_easy_cleanup(curl_price);
            }
            free(price_response.buffer);
            
            if (trade_status == 1) { 
                sleep(5); 
            }
            continue; // PULA O MENU E VOLTA PARA O TOPO PARA MONITORAR DE NOVO
        } 
        // Bloco de monitoramento de VENDA ÚNICA (status 2)
        else if (trade_status == 2) 
        { 
            printf("\n[MONITORANDO VENDA ÚNICA] Alvo para %s: %.8f\n", trade_symbol, sell_target_price);
            
            char price_endpoint_url[256];
            snprintf(price_endpoint_url, sizeof(price_endpoint_url), "https://api.binance.com/api/v3/ticker/price?symbol=%s", trade_symbol);

            CURL *curl_price;
            CURLcode res_price;
            struct Memory price_response;
            price_response.buffer = malloc(1);
            price_response.size = 0;

            curl_price = curl_easy_init();
            if (curl_price) {
                curl_easy_setopt(curl_price, CURLOPT_CAINFO, "cacert.pem"); // Certificado SSL
                curl_easy_setopt(curl_price, CURLOPT_URL, price_endpoint_url);
                curl_easy_setopt(curl_price, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
                curl_easy_setopt(curl_price, CURLOPT_WRITEDATA, (void *)&price_response);
                res_price = curl_easy_perform(curl_price);

                if (res_price == CURLE_OK) {
                    cJSON *json = cJSON_Parse(price_response.buffer);
                    if (json != NULL) {
                        cJSON *price_item = cJSON_GetObjectItemCaseSensitive(json, "price");
                        if (cJSON_IsString(price_item) && (price_item->valuestring != NULL)) {
                            double current_price = atof(price_item->valuestring);
                            printf("Preço atual: %.8f\n", current_price);

                            if (current_price >= sell_target_price) { 
                                printf("\n!!! ALVO DE VENDA ÚNICA ATINGIDO !!!\n");
                                place_limit_sell_order(trade_symbol, sell_target_price, quantity);
                                trade_status = 0; 
                                strcpy(trade_symbol, "");
                            }
                        } else {
                             printf("ERRO: Não foi possível ler a chave 'price'. Verifique o par de moedas.\n");
                        }
                        cJSON_Delete(json);
                    }
                } else {
                    fprintf(stderr, "Erro ao buscar preço: %s\n", curl_easy_strerror(res_price));
                }
                curl_easy_cleanup(curl_price);
            }
            free(price_response.buffer);
            
            if (trade_status == 2) { 
                sleep(5); 
            }
            continue; // PULA O MENU E VOLTA PARA O TOPO PARA MONITORAR DE NOVO
        }
        // Bloco de monitoramento do LOOP - AGUARDANDO COMPRA (status 3)
        else if (trade_status == 3) { 
            printf("\n[LOOP - AGUARDANDO COMPRA] Alvo: %.8f (Venda em %.8f)\n", buy_target_price, sell_target_price);
            
            char price_endpoint_url[256];
            snprintf(price_endpoint_url, sizeof(price_endpoint_url), "https://api.binance.com/api/v3/ticker/price?symbol=%s", trade_symbol);

            CURL *curl_price;
            CURLcode res_price;
            struct Memory price_response;
            price_response.buffer = malloc(1);
            price_response.size = 0;

            curl_price = curl_easy_init();
             if (curl_price) {
                curl_easy_setopt(curl_price, CURLOPT_CAINFO, "cacert.pem"); // Certificado SSL
                curl_easy_setopt(curl_price, CURLOPT_URL, price_endpoint_url);
                curl_easy_setopt(curl_price, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
                curl_easy_setopt(curl_price, CURLOPT_WRITEDATA, (void *)&price_response);
                res_price = curl_easy_perform(curl_price);

                if (res_price == CURLE_OK) {
                    cJSON *json = cJSON_Parse(price_response.buffer);
                    if (json != NULL) {
                        cJSON *price_item = cJSON_GetObjectItemCaseSensitive(json, "price");
                        if (cJSON_IsString(price_item) && (price_item->valuestring != NULL)) {
                            double current_price = atof(price_item->valuestring);
                            printf("Preço atual: %.8f\n", current_price);

                            if (current_price <= buy_target_price) {
                                printf("\n!!! LOOP: ALVO DE COMPRA ATINGIDO !!!\n");
                                place_limit_buy_order(trade_symbol, buy_target_price, quantity);
                                printf("Compra executada. Agora aguardando preço de venda...\n");
                                trade_status = 4; // MUDA O ESTADO PARA AGUARDAR VENDA
                            }
                        } else {
                             printf("ERRO: Não foi possível ler a chave 'price'. Verifique o par de moedas.\n");
                        }
                        cJSON_Delete(json);
                    }
                } else {
                    fprintf(stderr, "Erro ao buscar preço: %s\n", curl_easy_strerror(res_price));
                }
                curl_easy_cleanup(curl_price);
            }
            free(price_response.buffer);

            if (trade_status == 3) { // Só espera se ainda estiver aguardando compra
                sleep(5); 
            }
            continue; // PULA O MENU E VOLTA PARA O TOPO
        }
        // Bloco de monitoramento do LOOP - AGUARDANDO VENDA (status 4)
         else if (trade_status == 4) { 
            printf("\n[LOOP - AGUARDANDO VENDA] Alvo: %.8f (Compra em %.8f)\n", sell_target_price, buy_target_price);
           
            char price_endpoint_url[256];
            snprintf(price_endpoint_url, sizeof(price_endpoint_url), "https://api.binance.com/api/v3/ticker/price?symbol=%s", trade_symbol);

            CURL *curl_price;
            CURLcode res_price;
            struct Memory price_response;
            price_response.buffer = malloc(1);
            price_response.size = 0;

            curl_price = curl_easy_init();
            if (curl_price) {
                curl_easy_setopt(curl_price, CURLOPT_CAINFO, "cacert.pem"); // Certificado SSL
                curl_easy_setopt(curl_price, CURLOPT_URL, price_endpoint_url);
                curl_easy_setopt(curl_price, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
                curl_easy_setopt(curl_price, CURLOPT_WRITEDATA, (void *)&price_response);
                res_price = curl_easy_perform(curl_price);

                if (res_price == CURLE_OK) {
                    cJSON *json = cJSON_Parse(price_response.buffer);
                    if (json != NULL) {
                        cJSON *price_item = cJSON_GetObjectItemCaseSensitive(json, "price");
                        if (cJSON_IsString(price_item) && (price_item->valuestring != NULL)) {
                            double current_price = atof(price_item->valuestring);
                            printf("Preço atual: %.8f\n", current_price);

                            if (current_price >= sell_target_price) {
                                printf("\n!!! LOOP: ALVO DE VENDA ATINGIDO !!!\n");
                                place_limit_sell_order(trade_symbol, sell_target_price, quantity);
                                printf("Venda executada. Agora aguardando preço de compra...\n");
                                trade_status = 3; // MUDA O ESTADO PARA AGUARDAR COMPRA
                            }
                        } else {
                             printf("ERRO: Não foi possível ler a chave 'price'. Verifique o par de moedas.\n");
                        }
                        cJSON_Delete(json);
                    }
                } else {
                    fprintf(stderr, "Erro ao buscar preço: %s\n", curl_easy_strerror(res_price));
                }
                curl_easy_cleanup(curl_price);
            }
            free(price_response.buffer);

            if (trade_status == 4) { // Só espera se ainda estiver aguardando venda
                sleep(5); 
            }
            continue; // PULA O MENU E VOLTA PARA O TOPO
        }
		
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
      // Carrega as variaveis de ambiente
      char *api_key = getenv("BINANCE_API_KEY");
      char *secret_key = getenv("BINANCE_SECRET_KEY");
     
      if (api_key == NULL || secret_key == NULL) 
	    {
        printf("Erro: API Key ou Secret Key não encontradas. Verifique o arquivo config.env.\n");
        return 1;
        }
     
      CURL *curl;
      CURLcode res;
     
      struct Memory response;
      response.buffer = malloc(1); // Inicializa o buffer
      response.size = 0;
		  
      curl = curl_easy_init();

      if (curl) 
	    {
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

        // Configurar os cabeçalhos
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

        if (res != CURLE_OK) 
	      {
          fprintf(stderr, "Erro ao acessar a API Binance: %s\n", curl_easy_strerror(res));
          } 
		else 
	      {
          // Chamar a função para imprimir o saldo de BTC e o ID da conta
          print_balances_and_account_id(response.buffer);
            }

        // Liberar recursos
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        }

      free(response.buffer);
      } 
    // Lógica para a Opção 2: Trade
    else if (opcao == 2)
      {
	  if (trade_status != 0) { // Se uma trade (qualquer tipo) está ativa
                printf("\n--- STATUS DA TRADE ATIVA ---\n");
                printf("Par: %s\n", trade_symbol);
                printf("Quantidade: %.8f\n", quantity); 
                if (trade_status == 1) {
                    printf("Ação: AGUARDANDO COMPRA ÚNICA ATINGIR %.8f\n", buy_target_price); // Usar a variável correta
                } else if (trade_status == 2) {
                    printf("Ação: AGUARDANDO VENDA ÚNICA ATINGIR %.8f\n", sell_target_price); // Usar a variável correta
                } else if (trade_status == 3) {
                    printf("Ação: LOOP - AGUARDANDO COMPRA ATINGIR %.8f (Venda em %.8f)\n", buy_target_price, sell_target_price);
                } else if (trade_status == 4) {
                    printf("Ação: LOOP - AGUARDANDO VENDA ATINGIR %.8f (Compra em %.8f)\n", sell_target_price, buy_target_price);
                }
                printf("---------------------------\n");
            } else { // Nenhuma trade ativa, configurar uma nova
                int trade_choice = 0;
                printf("\n--- CONFIGURAR NOVA ORDEM ---\n");
                printf("1 - Ordem de Compra Única\n");
                printf("2 - Ordem de Venda Única\n");
                printf("3 - Loop Automático Compra/Venda\n"); // Nova opção
                printf("Escolha uma opção: ");
                scanf(" %d", &trade_choice);

                if (trade_choice == 1) // Compra Única
                {
                    printf("\n--- NOVA ORDEM DE COMPRA ÚNICA ---\n");
                    printf("Digite o par de moedas (ex: BTCUSDT): ");
                    scanf("%s", trade_symbol);
                    printf("Digite o preço de compra desejado: ");
                    scanf("%lf", &buy_target_price); // Guardar em buy_target_price
                    printf("Digite a quantidade que deseja comprar: ");
                    scanf("%lf", &quantity);
                    trade_status = 1; 
                    printf("\nOrdem configurada! Iniciando monitoramento...\n");
                } 
                else if (trade_choice == 2) // Venda Única
                {
                    printf("\n--- NOVA ORDEM DE VENDA ÚNICA ---\n");
                    printf("Digite o par de moedas (ex: BTCUSDT): ");
                    scanf("%s", trade_symbol);
                    printf("Digite o preço de venda desejado: ");
                    scanf("%lf", &sell_target_price); // Guardar em sell_target_price
                    printf("Digite a quantidade que deseja vender: ");
                    scanf("%lf", &quantity);
                    trade_status = 2; 
                    printf("\nOrdem configurada! Iniciando monitoramento...\n");
                }
                else if (trade_choice == 3) // Loop Compra/Venda
                {
                    printf("\n--- CONFIGURAR LOOP COMPRA/VENDA ---\n");
                    printf("Digite o par de moedas (ex: BTCUSDT): ");
                    scanf("%s", trade_symbol);
                    printf("Digite o PREÇO ALVO para COMPRAR (valor baixo): ");
                    scanf("%lf", &buy_target_price);
                    printf("Digite o PREÇO ALVO para VENDER (valor alto): ");
                    scanf("%lf", &sell_target_price);
                    printf("Digite a QUANTIDADE a ser negociada em cada ciclo: ");
                    scanf("%lf", &quantity);

                    // Validar se o preço de venda é maior que o de compra
                    if (sell_target_price <= buy_target_price) {
                        printf("ERRO: O preço de venda deve ser maior que o preço de compra.\n");
                    } else {
                        int start_choice = 0;
                        printf("Deseja começar tentando COMPRAR (1) ou VENDER (2)? ");
                        scanf(" %d", &start_choice);

                        if (start_choice == 1) {
                            trade_status = 3; // Inicia aguardando compra
                            printf("\nLoop configurado! Iniciando monitoramento para comprar a %.8f...\n", buy_target_price);
                        } else if (start_choice == 2) {
                            trade_status = 4; // Inicia aguardando venda
                            printf("\nLoop configurado! Iniciando monitoramento para vender a %.8f...\n", sell_target_price);
                        } else {
                            printf("Opção de início inválida.\n");
                        }
                    }
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
