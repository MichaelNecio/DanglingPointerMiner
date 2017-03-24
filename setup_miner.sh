# Our certificate must be in ~/.cscert/ca.pem
source container/env.rc
docker --config ./container/ build ./container/miner/
docker --config ./container/ create UniversityOfWindsor
docker --config ./container/ start UniversityOfWindsor
