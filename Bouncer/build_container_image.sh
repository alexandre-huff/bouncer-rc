#!/bin/bash

set -euo pipefail

usage="$(basename "$0") [-h] [-r <repo:tag>] [-L <host:port>] -- build and push bouncer-rc image (always pushes to both registries)

where:
    -h  show this help text
    -r  set Docker Hub image repo:tag (default: zanattabruno/bouncer-rc:TNSM-25)
    -L  set local registry host:port to also tag/push (default: registry-docker-registry.registry.svc.cluster.local:5000)"

# Defaults
repotag="zanattabruno/bouncer-rc:TNSM-25"
LOCAL_REGISTRY="registry-docker-registry.registry.svc.cluster.local:5000"

if [ "${*:-}" == "" ]; then
    echo "No flags passed. Using defaults: repotag=$repotag, local-registry=$LOCAL_REGISTRY"
fi

while getopts ":hr:L:" flag; do
        case "${flag}" in
        r) repotag=${OPTARG} ;;
        L) LOCAL_REGISTRY=${OPTARG} ;;
        h)
            echo "$usage"
            exit 0
            ;;
        :) echo "Option -$OPTARG requires an argument"; echo "$usage"; exit 1 ;;
            \?) echo "Invalid option: -$OPTARG"; echo "$usage"; exit 1 ;;
        esac
done

# Ensure Docker is installed
if ! command -v docker >/dev/null 2>&1; then
    echo "Docker is not installed. Please install Docker and try again."
    exit 1
fi

# Helper to check registry connectivity (best-effort)
check_registry() {
    local registry_url=$1
    local registry_name=$2

    echo "Checking connectivity to $registry_name..."
    if command -v curl >/dev/null 2>&1; then
        if curl -s -f "http://$registry_url/v2/" >/dev/null 2>&1; then
            echo "$registry_name is accessible."
            return 0
        else
            echo "Warning: $registry_name ($registry_url) is not accessible."
            return 1
        fi
    else
        echo "curl not found; skipping connectivity pre-check for $registry_name."
        return 0
    fi
}

# Derive components
image_path_no_tag=${repotag%:*}
image_tag=${repotag##*:}
base_image_name=${image_path_no_tag##*/}
local_image_tag="${LOCAL_REGISTRY}/${base_image_name}:${image_tag}"

echo "Building Docker image..."
docker build . -t "$repotag"

echo "Tagging image for local registry..."
docker tag "$repotag" "$local_image_tag"

echo "=== Pushing to Docker Hub ==="
# Always attempt to push; do not abort if it fails
set +e
docker push "$repotag"
hub_status=$?

echo "=== Pushing to Local Registry ($LOCAL_REGISTRY) ==="
docker push "$local_image_tag"
local_status=$?
set -e

echo
echo "=== Summary ==="
if [ "$hub_status" -eq 0 ]; then
    echo "Docker Hub: $repotag (pushed)"
else
    echo "Docker Hub: $repotag (FAILED)"
fi
if [ "$local_status" -eq 0 ]; then
    echo "Local Registry: $local_image_tag (pushed)"
else
    echo "Local Registry: $local_image_tag (FAILED)"
fi

# Exit non-zero if any push failed
if [ "$hub_status" -ne 0 ] || [ "$local_status" -ne 0 ]; then
    exit 1
fi