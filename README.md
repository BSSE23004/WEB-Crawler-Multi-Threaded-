Multi-threaded Web Crawler README
How to Build and Run

Install Dependencies:
Install libcurl: sudo apt-get install libcurl4-openssl-dev (on Ubuntu).
Ensure a C compiler (e.g., gcc) is installed.


Compile:gcc -o crawler crawler.c -lcurl -lpthread

For Plotting : python3 plot_script.py


Run:./crawler urls.txt <num_threads> <chunk_size>


./crawler urls.txt 1 100
./crawler urls.txt 2 50
./crawler urls.txt 4 25
./crawler urls.txt 8 100

<url_file>: Path to the text file with URLs (e.g., urls.txt).
<num_threads>: Number of threads (e.g., 2, 4, 8).
<chunk_size>: Number of words per thread (not implemented; use any integer, e.g., 1000).



Libraries Used

libcurl: For HTTP requests to download URL content.
pthreads: For multi-threading support.

Implemented Statistics

Sentence count: Counts sentences per URL (delimited by '.', '?', '!').
Character frequency: Counts a-z occurrences (case-insensitive) across all URLs.
Words per URL: Counts total words per URL.

Notes

Each thread processes one URL entirely.
The chunk_size argument is included but not used in this version.

