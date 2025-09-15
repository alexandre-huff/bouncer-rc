# Bouncer xApp

This repository mirrors the O-RAN SC Bouncer xApp and provides a C++ implementation of an xApp that speaks E2SM-RC (and basic KPM) over the Near-RT RIC. It builds a single binary `b_xapp_main` and includes container and Helm artifacts for deployment.

Maintainer: Alexandre Huff <alexandrehuff@utfpr.edu.br>


## What this project does

- Sends E2 subscriptions (primarily E2SM-RC; basic E2SM-KPM supported) to connected E2 nodes
- Receives and parses RIC Indications, extracting UE/Cell metrics and exposing Prometheus metrics
- Talks to RIC platform services (E2Mgr, SubMgr) over HTTP, and transports E2 messages over RMR


## Repository layout

- `Bouncer/src/` – C++ sources for the xApp (entrypoint `b_xapp_main`)
- `Bouncer/asn1c_defs/` – ASN.1 generated code for E2AP and E2SM-RC
- `Bouncer/Dockerfile` – Multi-stage image to build and package the xApp
- `bouncer-xapp/` – Helm chart for deploying the xApp to a Near-RT RIC cluster
- `Bouncer/init/` – Example config files (xApp descriptor, env script, routes)
- `docs/` – Sphinx docs scaffolding (optional)

Key files to start with:

- `Bouncer/src/b_xapp_main.cc` – startup/shutdown, threads, signals
- `Bouncer/src/xapp.cc` – SubMgr/E2Mgr interactions; RC/KPM subscription builders
- `Bouncer/src/xapp-mgmt/msgs_proc.cc` – message handling; RC Indication parsing; metrics
- `Bouncer/src/xapp-utils/xapp_config.*` – configuration model and precedence


## Architecture (high level)

- Runtime entrypoint (`b_xapp_main`):
	- Loads settings from defaults, environment, xApp descriptor (`/opt/ric/config/config-file.json`), then CLI
	- Initializes RMR, starts a receive loop, and starts an HTTP listener used by SubMgr callbacks
	- Startup: discover connected NodeBs via E2Mgr, send xApp registration, send E2 subscriptions, fetch A1 policies
	- Shutdown: delete subscriptions and deregister
- Messaging (RMR): send/receive via `xapp_rmr_*`; handlers can request RTS of the same mbuf
- Platform services (HTTP inside cluster DNS):
	- SubMgr: `http://service-ricplt-submgr-http.ricplt.svc.cluster.local:8088`
	- E2Mgr: `http://service-ricplt-e2mgr-http.ricplt.svc.cluster.local:3800`
- Metrics: Prometheus endpoint exposed; metrics updated from RC Indications


## Prerequisites

Local native build requires a recent Linux (tested with Ubuntu 24.04) and system packages comparable to what the Dockerfile installs:

- g++14 or newer, make
- rmr_si (RMR), mdclog
- cpprestsdk, prometheus-cpp, boost

Tip: If you don’t want to set up native dependencies, use the provided Dockerfile or the Helm chart.


## Quick start: run locally (native)

1) Prepare environment and routes

- Ensure an RMR route table exists. Examples:
	- `Bouncer/src/routes.txt` (local dev) or `/etc/xapp/routes.txt`
- Export base environment. For convenience:
	- `source Bouncer/src/xapp_env.sh` or `source Bouncer/init/xapp_env.sh`

2) Build and run

- From `Bouncer/src/` run `make` to build `b_xapp_main`
- Start the binary; optionally filter to a specific NodeB with `-b/--nodebid` plus `-c/--mcc -n/--mnc`

Example commands (optional):

```bash
# Set up env for local dev
source Bouncer/src/xapp_env.sh

# Build
make -C Bouncer/src

# Run (subscribe to a specific NodeB)
./Bouncer/src/b_xapp_main -b 12 -c 001 -n 01
```

Notes:

- RMR blocks until routing is ready. Ensure `RMR_SEED_RT` points to your routes.txt
- `RMR_SRC_ID` must be set for proper HTTP callback address derivation; otherwise SubMgr callbacks will fail


## Build and run in a container

Use the helper script which builds with the multi-stage Dockerfile and optionally pushes to a local registry.

```bash
# Build and tag the image, optionally push to a local registry
./Bouncer/build_container_image.sh -r <repo:tag> -L <local-registry:port>
```

Runtime defaults (inside the image):

- `CMD ["b_xapp_main"]`
- `RMR_SEED_RT=/etc/xapp/routes.txt`
- `RMR_RTG_SVC=9999`
- `CONFIG_MAP_NAME=/etc/xapp/config-map.yaml`


## Deploy with Helm

The chart lives in `bouncer-xapp/`.

```bash
# Example (adjust registry/tag/namespace to your cluster)
helm upgrade --install bouncer-xapp ./bouncer-xapp \
	--namespace ricxapp --create-namespace \
	--set image.repository=<registry>/bouncer-rc \
	--set image.tag=<tag>
```

Chart defaults (`bouncer-xapp/values.yaml`) set ports (HTTP 8080, RMR 4560), RMR rx/tx message types, and Prometheus scraping.


## Configuration

Configuration sources and precedence: defaults < environment < xApp descriptor (`/opt/ric/config/config-file.json`) < CLI.

Common settings (see `Bouncer/src/xapp-utils/xapp_config.*`):

- `XAPP_ID`, `XAPP_NAME`
- Ports: `BOUNCER_PORT` (RMR), `HTTP_PORT`
- `RMR_SRC_ID` (required to derive HTTP callback address)
- `THREADS`, `MSG_MAX_BUFFER`
- `CONFIG_FILE` (defaults to `/opt/ric/config/config-file.json`)
- Optional NodeB filter: `MCC`, `MNC`, `NODEB_ID` (32-bit bitstring; PLMN constructed per 3GPP nibble order)

Additional runtime env used by the container/Helm:

- `RMR_SEED_RT` path to routes file (must exist; RMR blocks until routes are ready)
- `RMR_RTG_SVC` routing service port (default 9999)
- `CONFIG_MAP_NAME` for YAML with logging and other settings (see `Bouncer/init/config-map.yaml`)


## Development notes

- Switching RC vs KPM subscription: in `Xapp::startup_subscribe_requests` inside `Bouncer/src/xapp.cc`, use `build_rc_subscription_request` (default) or the alternate `build_kpm_subscription_request` shown in code comments
- NodeB filtering: `./b_xapp_main -b <nodeb-id> -c <mcc> -n <mnc>` subscribes only to that NodeB
- Logging uses mdclog; level is configured via YAML in `CONFIG_MAP_NAME`
- Metrics are exported for Prometheus scraping at the service’s `/metrics`


## Troubleshooting and gotchas

- SubMgr callbacks failing: ensure `RMR_SRC_ID` is set; the HTTP source ID is derived from it by replacing `-rmr.` with `-http.`
- RMR stuck waiting: verify `RMR_SEED_RT` points to a valid routes table and that a router is reachable
- E2Mgr/SubMgr addresses assume in-cluster DNS. For local runs, mock or port-forward the services (see values in Architecture section)


## Building docs (optional)

Sphinx docs live under `docs/` and can be built with tox:

```bash
tox -e docs
```


## License and upstream

This project mirrors O-RAN SC Bouncer. Source code is Apache 2.0 licensed. See upstream for original history and context.


## Contributing

Please see `CONTRIBUTING.md` for guidelines. Issues and PRs are welcome.


---

If anything above is unclear or you need more examples (e.g., A1 payload shape, metrics names), open an issue and we’ll refine the docs.

