# DROP TABLE github;

CREATE TABLE github (
    push_id bigint,
    repo_id bigint,
    ref text,
    head text,
    before text,
    actor_info text,
    repo_info text,
    PRIMARY KEY (repo_id, push_id)
);
