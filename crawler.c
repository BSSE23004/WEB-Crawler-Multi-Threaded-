#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include <ctype.h>
#include <time.h>

#define MAX_URLS 100
#define MAX_URL_LEN 256
#define MAX_CONTENT_LEN 1024 * 1024  // 1MB max content size

// Global variables
char urls[MAX_URLS][MAX_URL_LEN];
int url_count = 0;
pthread_mutex_t url_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
int url_index = 0;
int char_freq[26] = {0};  // Frequency of a-z
int *sentence_counts = NULL;
int *word_counts = NULL;

// CURL write callback to store downloaded data
struct MemoryStruct {
    char *memory;
    size_t size;
};

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Function to download a URL
char *download_url(const char *url) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  // Will be grown by realloc
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            chunk.memory = NULL;
        }
        curl_easy_cleanup(curl);
    }
    return chunk.memory;
}

// Helper function to count sentences
int count_sentences(const char *text) {
    int count = 0;
    for (int i = 0; text[i]; i++) {
        if (text[i] == '.' || text[i] == '!' || text[i] == '?') {
            count++;
        }
    }
    return count;
}

// Helper function to update character frequency
void update_char_freq(const char *text) {
    for (int i = 0; text[i]; i++) {
        if (isalpha(text[i])) {
            char_freq[tolower(text[i]) - 'a']++;
        }
    }
}

// Helper function to count words
int count_words(const char *text) {
    int count = 0;
    int in_word = 0;
    for (int i = 0; text[i]; i++) {
        if (isspace(text[i])) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            count++;
        }
    }
    return count;
}

// Worker thread function
void *worker(void *arg) {
    while (1) {
        int idx;
        pthread_mutex_lock(&url_mutex);
        if (url_index >= url_count) {
            pthread_mutex_unlock(&url_mutex);
            break;
        }
        idx = url_index++;
        pthread_mutex_unlock(&url_mutex);

        char *content = download_url(urls[idx]);
        if (content) {
            pthread_mutex_lock(&stats_mutex);
            sentence_counts[idx] = count_sentences(content);
            word_counts[idx] = count_words(content);
            update_char_freq(content);
            pthread_mutex_unlock(&stats_mutex);
            free(content);
        }
    }
    return NULL;
}

// Function to read URLs from a file
void read_urls(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening URL file");
        exit(1);
    }
    while (url_count < MAX_URLS && fgets(urls[url_count], MAX_URL_LEN, file)) {
        urls[url_count][strcspn(urls[url_count], "\n")] = 0;  // Remove newline
        url_count++;
    }
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <url_file> <num_threads> <chunk_size>\n", argv[0]);
        return 1;
    }

    const char *url_file = argv[1];
    int num_threads = atoi(argv[2]);
    // int chunk_size = atoi(argv[3]);  // Not used in this version

    if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
        fprintf(stderr, "curl_global_init() failed\n");
        return 1;
    }

    read_urls(url_file);
    sentence_counts = calloc(url_count, sizeof(int));
    word_counts = calloc(url_count, sizeof(int));
    if (!sentence_counts || !word_counts) {
        fprintf(stderr, "Memory allocation failed\n");
        curl_global_cleanup();
        return 1;
    }

    pthread_t threads[num_threads];
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // Print results
    printf("\nResults:\n");
    for (int i = 0; i < url_count; i++) {
        printf("URL %d: %s\n", i + 1, urls[i]);
        printf("  Sentences: %d\n", sentence_counts[i]);
        printf("  Words: %d\n", word_counts[i]);
    }
    printf("\nCharacter Frequency (a-z):\n");
    for (int i = 0; i < 26; i++) {
        if (char_freq[i] > 0) {
            printf("%c: %d\n", 'a' + i, char_freq[i]);
        }
    }
    printf("\nTotal execution time: %.3f seconds\n", time_spent);

    free(sentence_counts);
    free(word_counts);
    curl_global_cleanup();
    return 0;
}