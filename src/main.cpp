#include "Response.hpp"
#include "Parsing.hpp"

int main(int ac, char **av, char **env) {
    if (ac != 2) {
        cerr << "usage: ./webserv [CONFIG]" << endl;
        return (1);
    }

    (void) ac;
    (void) av;
    (void) env;

    return (0);
}
