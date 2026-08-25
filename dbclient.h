#ifndef __DBCLIENT_H_
#define __DBCLIENT_H_

#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

#include "pieces.h"

class Dbclient {
public:
    bool write(std::vector<Pieces *> records);
    bool createTable();
    std::string getRecords();
};

#endif
