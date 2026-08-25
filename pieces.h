#ifndef __PIECES_H_
#define __PIECES_H_

#include <string>
#include <cstdint>

struct Pieces {
    uint64_t repoId;
    uint64_t pushId;
    std::string ref;
    std::string head;
    std::string before;
    std::string actorUrl;
    std::string repoUrl;
    std::string actorInfo;
    std::string repoInfo;

    Pieces(uint64_t repoId,
           uint64_t pushId,
           const std::string &ref,
           const std::string &head,
           const std::string &before,
           const std::string &repoUrl,
           const std::string &actorUrl) : repoId(repoId),
                                          pushId(pushId),
                                          ref(ref),
                                          head(head),
                                          before(before),
                                          repoUrl(repoUrl),
                                          actorUrl(actorUrl) {}
    };

#endif
