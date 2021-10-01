
docker run --rm -v "$GITHUB_WORKSPACE:/work" --env "CROSS=$CROSS" "dockcross/$CROSS" bash -c '/work/.scripts/build.sh'

