#include "ConfigParser.hpp"
#include "HTTPParser.hpp"
#include <iostream>
#include <cassert>
#include <sstream>

void test_basic_get() {
    std::cout << "Testing basic GET request..." << std::endl;
    
    char buff[] = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HTTPParser parser;
    
    parser.addChunk(buff, strlen(buff));
    
    assert(parser.getState() == COMPLETE);
    assert(parser.getMethod() == "GET");
    assert(parser.getUri() == "/index.html");
    assert(parser.getVers() == "HTTP/1.1");
    assert(parser.getHeader("host") == "localhost");
    
    std::cout << "✓ Basic GET test passed\n" << std::endl;
}

void test_post_with_body() {
    std::cout << "Testing POST with body..." << std::endl;
    
    char buff[] = 
        "POST /submit HTTP/1.1\r\n"
        "Content-Length: 16\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n"
        "name=john&age=25";
    
    HTTPParser parser;
    parser.addChunk(buff, strlen(buff));
    assert(parser.getState() == COMPLETE);
    assert(parser.getMethod() == "POST");
    assert(parser.getBody() == "name=john&age=25");
    assert(parser.getHeader("content-length") == "16");
    
    std::cout << "✓ POST with body test passed\n" << std::endl;
}

void test_chunked_parsing() {
    std::cout << "Testing chunked parsing..." << std::endl;
    
    HTTPParser parser;
    
    // Send request line first
    char chunk1[] = "GET /test HTTP/1.1\r\n";
    parser.addChunk(chunk1, strlen(chunk1));
    assert(parser.getState() == HEADERS);
    
    // Send headers
    char chunk2[] = "Host: example.com\r\nContent-Length: 5\r\n\r\n";
    parser.addChunk(chunk2, strlen(chunk2));
    assert(parser.getState() == BODY);
    
    // Send body
    char chunk3[] = "hello";
    parser.addChunk(chunk3, strlen(chunk3));
    assert(parser.getState() == COMPLETE);
    assert(parser.getBody() == "hello");
    
    std::cout << "✓ Chunked parsing test passed\n" << std::endl;
}

void test_case_insensitive_headers() {
    std::cout << "Testing case-insensitive headers..." << std::endl;
    
    char buff[] = "GET / HTTP/1.1\r\nContent-TYPE: text/html\r\nHOST: localhost\r\n\r\n";
    HTTPParser parser;
    
    parser.addChunk(buff, strlen(buff));
    
    assert(parser.getHeader("content-type") == "text/html");
    assert(parser.getHeader("host") == "localhost");
    
    std::cout << "✓ Case-insensitive headers test passed\n" << std::endl;
}

void test_whitespace_handling() {
    std::cout << "Testing whitespace in headers..." << std::endl;
    
    char buff[] = "GET / HTTP/1.1\r\nContent-Type:   text/html   \r\nHost:localhost\r\n\r\n";
    HTTPParser parser;
    
    parser.addChunk(buff, strlen(buff));
    
    assert(parser.getHeader("content-type") == "text/html");
    assert(parser.getHeader("host") == "localhost");
    
    std::cout << "✓ Whitespace handling test passed\n" << std::endl;
}

void test_invalid_requests() {
    std::cout << "Testing invalid requests..." << std::endl;
    
    // Test missing HTTP version
    {
        char buff[] = "GET /test\r\n\r\n";
        HTTPParser parser;
        parser.addChunk(buff, strlen(buff));
        assert(parser.getState() == ERROR);
    }
    
    // Test header without colon
    {
        char buff[] = "GET /test HTTP/1.1\r\nInvalidHeader\r\n\r\n";
        HTTPParser parser;
        parser.addChunk(buff, strlen(buff));
        assert(parser.getState() == ERROR);
    }
    
    // Test invalid content-length
    {
        char buff[] = "POST /test HTTP/1.1\r\nContent-Length: abc\r\n\r\n";
        HTTPParser parser;
        parser.addChunk(buff, strlen(buff));
        assert(parser.getState() == ERROR);
    }
    
    std::cout << "✓ Invalid request tests passed\n" << std::endl;
}

void test_reset_functionality() {
    std::cout << "Testing reset functionality..." << std::endl;
    
    char buff[] = "GET /test HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HTTPParser parser;
    
    parser.addChunk(buff, strlen(buff));
    assert(parser.getState() == COMPLETE);
    
    parser.reset();
    assert(parser.getState() == START_LINE);
    assert(parser.getMethod().empty());
    assert(parser.getUri().empty());
    assert(parser.getHeaders().empty());
    
    // Reuse parser
    parser.addChunk(buff, strlen(buff));
    assert(parser.getState() == COMPLETE);
    assert(parser.getMethod() == "GET");
    
    std::cout << "✓ Reset functionality test passed\n" << std::endl;
}

void test_large_body() {
    std::cout << "Testing large body..." << std::endl;

    std::string large_body(4 * 1024 * 1024, 'A'); // 4MB of 'A's

    std::stringstream ss;
    ss << large_body.size();

    std::string request = "POST /upload HTTP/1.1\r\nContent-Length: " +
                         ss.str() + "\r\n\r\n" + large_body;

    HTTPParser parser;
    parser.addChunk(const_cast<char*>(request.c_str()), request.size());

    assert(parser.getState() == COMPLETE);
    assert(parser.getBody().size() == 4 * 1024 * 1024);
    assert(parser.getBody() == large_body);

    std::cout << "✓ Large body test passed\n" << std::endl;
}

int main(int ac, char **av) {

    if (ac != 2)
    {
        return (EXIT_FAILURE);
    }
    WebConfigFile config;

    parseConfigFile(config, av[1]);

    // std::cout << "Running HTTPParser Test Suite\n" << std::endl;
    
    // test_basic_get();
    // test_post_with_body();
    // test_chunked_parsing();
    // test_case_insensitive_headers();
    // test_whitespace_handling();
    // test_invalid_requests();
    // test_reset_functionality();
    // test_large_body();
    
    // std::cout << "🎉 All tests passed!" << std::endl;
    
    return 0;
}