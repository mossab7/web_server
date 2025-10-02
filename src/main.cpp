#include <iostream>
#include <csignal>
#include <exception>
#include "../include/utils/Logger.hpp"

// Forward declaration of the event_loop function
void event_loop();

// Global flag for graceful shutdown
volatile sig_atomic_t g_shutdown = 0;

/**
 * @brief Signal handler for graceful shutdown
 * @param signal The signal number
 */
void signal_handler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\nReceived shutdown signal. Stopping server..." << std::endl;
        g_shutdown = 1;
    }
}

/**
 * @brief Setup signal handlers for graceful shutdown
 */
void setup_signal_handlers()
{
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        throw std::runtime_error("Failed to setup SIGINT handler");
    }
    
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        throw std::runtime_error("Failed to setup SIGTERM handler");
    }
    
    // Ignore SIGPIPE to handle broken pipe errors gracefully
    signal(SIGPIPE, SIG_IGN);
}

/**
 * @brief Main function to test the event loop
 * @param argc Number of command line arguments (unused)
 * @param argv Command line arguments (unused)
 * @return Exit status
 */
int main(int /* argc */, char* /* argv */[])
{
    try
    {
        // Initialize logger
        Logger logger;
        logger.info("Starting webserver...");
        
        // Setup signal handlers for graceful shutdown
        setup_signal_handlers();
        logger.info("Signal handlers configured");
        
        // Print startup message
        std::cout << "=== Webserver Starting ===" << std::endl;
        std::cout << "Press Ctrl+C to stop the server gracefully" << std::endl;
        std::cout << "Listening for connections..." << std::endl;
        
        // Start the event loop
        logger.info("Starting event loop");
        event_loop();
        
        // This point should not be reached unless event_loop exits
        logger.info("Event loop exited");
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
    
    std::cout << "Server stopped gracefully" << std::endl;
    return 0;
}
// =======
// #include "ConfigParser.hpp"
// #include "Routing.hpp"
// #include "HTTPParser.hpp"
// #include "Response.hpp"
// #include "error_pages.hpp"

// int main(int ac, char **av)
// {
//     if (ac != 2)
//     {
//         cerr << "usage: ./webserv [CONFIG]" << endl;
//         return (EXIT_FAILURE);
//     }

//     initErrorPages(); 

//     WebConfigFile config;

//     if (parseConfigFile(config, av[1]))
//         return (1);

//     Routing routing(config);
//     HTTPResponse resp;

//     resp = handleRequest(routing, "localhost:8080", "/default.conf", "GET");

//     return (0);
// }






// ============= 
/*
#include "HTTPParser.hpp"
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// ==================== File Upload Handler ====================
struct FileUploadContext
{
    std::ofstream file;
    size_t bytes_written;
    size_t max_size;
    
    FileUploadContext(const char* filepath, size_t max = 10*1024*1024)
        : bytes_written(0), max_size(max)
    {
        file.open(filepath, std::ios::binary);
    }
    
    ~FileUploadContext()
    {
        if (file.is_open())
            file.close();
    }
};

void fileUploadHandler(const char* buff, size_t size, void* data)
{
    FileUploadContext* ctx = static_cast<FileUploadContext*>(data);
    
    if (!ctx->file.is_open())
    {
        std::cerr << "Error: File not open!" << std::endl;
        return;
    }
    
    if (ctx->bytes_written + size > ctx->max_size)
    {
        std::cerr << "Error: File size limit exceeded!" << std::endl;
        return;
    }
    
    ctx->file.write(buff, size);
    ctx->bytes_written += size;
    
    std::cout << "[UPLOAD] Written " << size << " bytes (total: " 
              << ctx->bytes_written << ")" << std::endl;
}

// ==================== CGI Input Handler ====================
struct CGIContext
{
    int pipe_fd;
    size_t bytes_written;
    
    CGIContext(int fd) : pipe_fd(fd), bytes_written(0) {}
};

void cgiInputHandler(const char* buff, size_t size, void* data)
{
    CGIContext* ctx = static_cast<CGIContext*>(data);
    
    ssize_t written = write(ctx->pipe_fd, buff, size);
    if (written < 0)
    {
        std::cerr << "Error: Failed to write to CGI pipe!" << std::endl;
        return;
    }
    
    ctx->bytes_written += written;
    std::cout << "[CGI] Written " << written << " bytes to pipe (total: " 
              << ctx->bytes_written << ")" << std::endl;
}

// ==================== Test Helper Functions ====================
void printRequestInfo(HTTPParser& parser)
{
    std::cout << "\n=== Request Info ===" << std::endl;
    std::cout << "Method : " << parser.getMethod() << std::endl;
    std::cout << "URI    : " << parser.getUri() << std::endl;
    std::cout << "Version: " << parser.getVers() << std::endl;
    
    std::cout << "\nHeaders:" << std::endl;
    strmap headers = parser.getHeaders();
    for (strmap::iterator it = headers.begin(); it != headers.end(); ++it)
        std::cout << "  [" << it->first << "]: " << it->second << std::endl;
    std::cout << std::endl;
}

bool readFileContent(const char* filepath, std::string& content)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
        return false;
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    content.resize(size);
    file.read(&content[0], size);
    file.close();
    
    return true;
}

// ==================== Test Cases ====================

void test_simple_file_upload()
{
    std::cout << "\n========== Test 1: Simple File Upload ==========" << std::endl;
    
    HTTPParser parser;
    FileUploadContext upload_ctx("/tmp/test_upload.txt");
    parser.setBodyHandler(fileUploadHandler, &upload_ctx);
    
    char request[] = 
        "POST /upload HTTP/1.1\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: 26\r\n"
        "\r\n"
        "This is test file content!";
    
    parser.addChunk(request, strlen(request));
    
    if (parser.isComplete())
    {
        printRequestInfo(parser);
        std::cout << "Upload complete! Bytes written: " << upload_ctx.bytes_written << std::endl;
        
        // Close file to flush buffer
        upload_ctx.file.close();
        
        // Verify file content
        std::string content;
        if (readFileContent("/tmp/test_upload.txt", content))
            std::cout << "File content: " << content << std::endl;
    }
    else
        std::cerr << "Error: Upload failed!" << std::endl;
}

void test_chunked_file_upload()
{
    std::cout << "\n========== Test 2: Chunked Transfer File Upload ==========" << std::endl;
    
    HTTPParser parser;
    FileUploadContext upload_ctx("/tmp/test_chunked_upload.txt");
    parser.setBodyHandler(fileUploadHandler, &upload_ctx);
    
    char request[] = 
        "POST /upload HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "d\r\n"              // 13 bytes in hex
        "Hello, World!"      // 13 bytes (no CRLF here!)
        "\r\n"               // Chunk trailing CRLF
        "19\r\n"             // 25 bytes in hex
        "This is chunked transfer."  // 26 bytes
        "\r\n"               // Chunk trailing CRLF
        "0\r\n"              // End
        "\r\n";
    
    parser.addChunk(request, strlen(request));
    
    if (parser.isComplete())
    {
        printRequestInfo(parser);
        std::cout << "Chunked upload complete! Bytes written: " << upload_ctx.bytes_written << std::endl;
        
        // Force flush by destroying context
        upload_ctx.file.close();
        
        std::string content;
        if (readFileContent("/tmp/test_chunked_upload.txt", content))
            std::cout << "File content: " << content << std::endl;
    }
    else
        std::cerr << "Error: Chunked upload failed!" << std::endl;
}

void test_large_file_simulation()
{
    std::cout << "\n========== Test 3: Large File Upload (Simulated) ==========" << std::endl;
    
    HTTPParser parser;
    FileUploadContext upload_ctx("/tmp/test_large_upload.bin");
    parser.setBodyHandler(fileUploadHandler, &upload_ctx);
    
    // Send headers first
    char headers[] = 
        "POST /upload HTTP/1.1\r\n"
        "Content-Length: 8192\r\n"
        "\r\n";
    
    parser.addChunk(headers, strlen(headers));
    
    // Simulate receiving file in chunks
    char chunk[1024];
    memset(chunk, 'A', sizeof(chunk));
    
    for (int i = 0; i < 8; ++i)
    {
        parser.addChunk(chunk, sizeof(chunk));
        std::cout << "Processed chunk " << (i+1) << "/8" << std::endl;
    }
    
    if (parser.isComplete())
    {
        std::cout << "\nLarge file upload complete! Total bytes: " 
                  << upload_ctx.bytes_written << std::endl;
    }
}

void test_cgi_input()
{
    std::cout << "\n========== Test 4: CGI POST Data ==========" << std::endl;
    
    // Create a pipe to simulate CGI stdin
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        std::cerr << "Failed to create pipe!" << std::endl;
        return;
    }
    
    HTTPParser parser;
    CGIContext cgi_ctx(pipefd[1]);  // Write end
    parser.setBodyHandler(cgiInputHandler, &cgi_ctx);
    
    char request[] = 
        "POST /cgi-bin/process.php HTTP/1.1\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 29\r\n"
        "\r\n"
        "name=John&email=john@test.com";
    
    parser.addChunk(request, strlen(request));
    
    if (parser.isComplete())
    {
        printRequestInfo(parser);
        
        // Close write end and read from pipe
        close(pipefd[1]);
        
        char buffer[256];
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (n > 0)
        {
            buffer[n] = '\0';
            std::cout << "CGI received data: " << buffer << std::endl;
        }
        
        close(pipefd[0]);
    }
}

void test_cgi_chunked_input()
{
    std::cout << "\n========== Test 5: CGI with Chunked Transfer ==========" << std::endl;
    
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        std::cerr << "Failed to create pipe!" << std::endl;
        return;
    }
    
    HTTPParser parser;
    CGIContext cgi_ctx(pipefd[1]);
    parser.setBodyHandler(cgiInputHandler, &cgi_ctx);
    
    char request[] = 
        "POST /cgi-bin/upload.py HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "Hello\r\n"
        "7\r\n"
        " World!\r\n"
        "0\r\n"
        "\r\n";
    
    parser.addChunk(request, strlen(request));
    
    if (parser.isComplete())
    {
        printRequestInfo(parser);
        close(pipefd[1]);
        
        char buffer[256];
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (n > 0)
        {
            buffer[n] = '\0';
            std::cout << "CGI received chunked data: " << buffer << std::endl;
        }
        
        close(pipefd[0]);
    }
}

void test_no_handler_fallback()
{
    std::cout << "\n========== Test 6: No Handler (Default Behavior) ==========" << std::endl;
    
    HTTPParser parser;
    // No handler set - should use default _body storage
    
    char request[] = 
        "POST /test HTTP/1.1\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, World!";
    
    parser.addChunk(request, strlen(request));
    
    if (parser.isComplete())
    {
        printRequestInfo(parser);
        std::cout << "Body (stored in memory): " << parser.getBody() << std::endl;
    }
}

// ==================== Main ====================
int main()
{
    std::cout << "HTTP Parser Test Suite - File Upload & CGI Focus\n";
    std::cout << "================================================\n";
    
    test_simple_file_upload();
    test_chunked_file_upload();
    test_large_file_simulation();
    test_cgi_input();
    test_cgi_chunked_input();
    test_no_handler_fallback();
    
    std::cout << "\n================================================" << std::endl;
    std::cout << "All tests completed!" << std::endl;
    std::cout << "\nCheck /tmp/ for uploaded files:" << std::endl;
    std::cout << "  - test_upload.txt" << std::endl;
    std::cout << "  - test_chunked_upload.txt" << std::endl;
    std::cout << "  - test_large_upload.bin" << std::endl;
    
    return 0;
}

int g_shutdown;
*/
