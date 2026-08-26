#include <cstdlib>
#include <sstream>

#include <string.h>

#include "dbclient.h"
#include "logger.h"

std::string Dbclient::getRecords()
{
    // Determine which connection string to use; this is dependent on running in a container or not. 
    std::string conninfo ("postgresql://postgres:mysecret@db:5432/postgres");

    char* env_val = std::getenv("EVENT_TRACKER_IN_CONTAINER");
    if (env_val == nullptr) {
        conninfo = "dbname=postgres user=postgres password=mysecret host=localhost";
    }
    
    // Establish database connection 
    PGconn *conn = PQconnectdb(conninfo.c_str());

    if (PQstatus(conn) == CONNECTION_BAD) {
        LOG_ERROR("Unable to connect to postgres database.");
        PQfinish(conn);
        return std::string("");
    }

    // Execute a basic SQL query
    PGresult *res = PQexec(conn, "SELECT * FROM github");

    // Validate the query execution status
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Unable to query postgres database.");
        PQclear(res);
        PQfinish(conn);
        return std::string("");
    }

    // Create a json array to send back to the client. 
    std::stringstream ss;
    ss << '[';
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        char *push_id = PQgetvalue(res, i, 0);
        char *repo_id = PQgetvalue(res, i, 1);
        char *ref = PQgetvalue(res, i, 2);
        char *head = PQgetvalue(res, i, 3);
        char *before = PQgetvalue(res, i, 4);
        char *actor_info = PQgetvalue(res, i, 5);
        char *repo_info = PQgetvalue(res, i, 6);

        ss << "{\"push_id\": " << push_id << ", "
           << "\"push_id\": " << repo_id << ", "
           << "\"ref\": " << '"' << ref << '"' << ", "
           << "\"head\": " << '"' << head << '"' << ", "
           << "\"before\": " << '"' << before << '"' << ", "
           << "\"actor_info\": " << '"' << actor_info << '"' << ", "
           << "\"repo_info\": " << '"' << repo_info << '"' << '}';

        if (i != rows - 1) {
            ss << ',';
        }
            
    }
    ss << "]\n";

    /// Free result allocations and close connection
    PQclear(res);
    PQfinish(conn);

    return ss.str();
}

bool Dbclient::createTable()
{
    // Establish database connection
    std::string conninfo ("postgresql://postgres:mysecret@db:5432/postgres");

    char* env_val = std::getenv("EVENT_TRACKER_IN_CONTAINER");
    if (env_val == nullptr) {
        conninfo = "dbname=postgres user=postgres password=mysecret host=localhost";
    }
    
    PGconn *conn = PQconnectdb(conninfo.c_str());
    PGresult *res = NULL;

    if (PQstatus(conn) != CONNECTION_OK) {
        LOG_ERROR("postgres connection failed.");
        return false;
    }

    const char *query = "CREATE TABLE IF NOT EXISTS github (push_id bigint, repo_id bigint, ref text, head text, before text, actor_info text, repo_info text, PRIMARY KEY (repo_id, push_id));";

    // Execute the parameterized query
    res = PQexecParams(
        conn,
        query,        // The SQL statement
        0,            // Number of parameters ($1, $2, etc.)
        NULL,         // Let PostgreSQL infer parameter OIDs automatically
        NULL,         // Array of parameter values (as text)
        NULL,         // Lengths array (not required for null-terminated strings)
        NULL,         // Formats array (defaults to text format for all parameters)
        0             // Ask for the output result in text format
        );

    // Verify the execution status
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        Logger::getInstance().log(LogLevel::ERROR, "Creating github table failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return false;
    }

    PQclear(res);
    PQfinish(conn);
    return true;
}

bool Dbclient::write(std::vector<Pieces *> records) {

    std::string conninfo ("postgresql://postgres:mysecret@db:5432/postgres");

    char* env_val = std::getenv("EVENT_TRACKER_IN_CONTAINER");
    if (env_val == nullptr) {
        conninfo = "dbname=postgres user=postgres password=mysecret host=localhost";
    }
    
    PGconn *conn = PQconnectdb(conninfo.c_str());
    PGresult *res = NULL;

    if (PQstatus(conn) != CONNECTION_OK) {
        Logger::getInstance().log(LogLevel::ERROR, "Creating github table failed: %s", PQerrorMessage(conn));
        return false;
    }

    // Define the parameterized INSERT query
    const char *query = "INSERT INTO github (push_id, repo_id, ref, head, before, actor_info, repo_info) VALUES ($1, $2, $3, $4, $5, $6, $7) ON CONFLICT DO NOTHING;";

    int num_params = 7;
    const char *param_values[7];

    for (auto const &r : records) {

        // Map your parameters to an array of pointers
        param_values[0] = std::to_string(r->pushId).c_str();
        param_values[1] = std::to_string(r->repoId).c_str();
        param_values[2] = r->ref.c_str();
        param_values[3] = r->head.c_str();
        param_values[4] = r->before.c_str();
        param_values[5] = r->actorInfo.c_str();
        param_values[6] = r->repoInfo.c_str();

        // Execute the parameterized query
        res = PQexecParams(
            conn,
            query,        // The SQL statement
            num_params,   // Number of parameters ($1, $2, etc.)
            NULL,         // Let PostgreSQL infer parameter OIDs automatically
            param_values, // Array of parameter values (as text)
            NULL,         // Lengths array (not required for null-terminated strings)
            NULL,         // Formats array (defaults to text format for all parameters)
            0             // Ask for the output result in text format
            );

        // Verify the execution status
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            Logger::getInstance().log(LogLevel::ERROR, "INSERT failed: %s", PQerrorMessage(conn));
            PQclear(res);
            PQfinish(conn);
            return false;
        }

    }
    
    PQclear(res);
    PQfinish(conn);
    return true;
}

