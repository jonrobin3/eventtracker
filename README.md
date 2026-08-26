# eventtracker
# Notes
You will see PushEvents being ingested and processed shortly after starting the event-tracker container. However, in order to work around Github’s rate limiting, new data will only be injected once every thirty minutes. 

This code has only been tested in Ubuntu 22.04. 

# To build the code and run outside of Docker
```
g++ -std=c++17 -I/usr/include/postgresql dbclient.cpp httpserver.cpp  fetcher.cpp ratelimiter.cpp service.cpp -lcurl -ljansson -lpq

./a.out
```

# To create a new image
```
g++ -std=c++17 -I/usr/include/postgresql dbclient.cpp httpserver.cpp  fetcher.cpp ratelimiter.cpp service.cpp -lcurl -ljansson -lpq
docker build -t event-tracker:latest --no-cache .
```

# To run the code in a Docker container and verify its operation
```
gh auth login
gh repo clone jonrobin3/eventtracker
docker-compose up -d --pull always
docker ps -a
docker logs <container-id>
```
You should see a large json array in the logs for the event-tracker container. This is the json array sent by queuing GitHub.com/Events. If you do not see the array in the docker log, there is an application file app.log which also stores the log information emitted by the service.

Log entries like the following tell you the service is placing metadata defined by this repo id and push id into the Postgres database. 
```
[2026-08-25 07:50:30] [INFO] Processing repo id= 1345285824 push id= 41149279326.
[2026-08-25 07:50:32] [INFO] Processing repo id= 1346051281 push id= 41166974654.
``` 

You can also inspect the database (container-id is the container running Postgres):
```
docker exec -it <container-id> bash
06d32cda8742:/# psql -U postgres -d postgres
06d32cda8742:/# select * from github;
```

You can verify rate limiting for our HttpServer is working correctly with the following. From the host running Docker:
```
jon@longwood:~/Desktop$ curl http://localhost:8080/
"visibility": "public",
  "forks": 0,
  "open_issues": 0,
  "watchers": 0,
  "default_branch": "main",
  "temp_clone_token": null,
  "network_count": 0,
  "subscribers_count": 0
}
"}]
jon@longwood:~/Desktop$ curl http://localhost:8080/
Too many requests.
```

The second request must be less than ten seconds from the first. After the ten seconds have expired, another curl request can be successfully sent. 
