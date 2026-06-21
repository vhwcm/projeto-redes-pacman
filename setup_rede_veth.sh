#!/bin/bash
echo "Criando cabos de rede virtuais (veth-srv e veth-cli)..."
ip link add veth-srv type veth peer name veth-cli 2>/dev/null || true
ip link set dev veth-srv up
ip link set dev veth-cli up
echo "Pronto! Redes criadas."
echo "Rode o servidor com: sudo ./server veth-srv"
echo "Rode o cliente com: sudo ./cliente veth-cli"
