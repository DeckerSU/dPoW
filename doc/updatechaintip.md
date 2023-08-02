### updatechaintip rpc

The `updatechaintip` RPC builds upon the original concept of `updatechaintip` introduced by Blackjok3r/[@wbpxre150](https://github.com/wbpxre150) and continued by [@who-biz](https://github.com/who-biz). More information can be found at [KomodoPlatform#445](https://github.com/KomodoPlatform/dPoW/pull/445). 

The main idea is to eliminate the need for the iguana to constantly poll
for `dpow_getchaintip` and `dpow_getbestblockhash` from `iguana_dPoWupdate` every 2 seconds, with a minimum interval of 1000 microseconds. Instead, we will initiate the `iguana_dPoWupdate` through an external event triggered by the daemon itself using blocknotify. To achieve this,
we can use the following blocknotify script in the .conf file of each daemon:

```
blocknotify=curl -s --url "http://127.0.0.1:7776" --data "{\"agent\":\"dpow\",\"method\":\"updatechaintip\",\"blockhash\":\"%s\",\"symbol\":\"COIN\"}"
```

In this example, `"COIN"` represents the coin ticker symbol.

This approach reduces CPU resource consumption and the number of `getbestblockhash` requests, resulting in decreased loopback/localhost traffic. Nodes implementing this approach should
also participate in a higher number of notarisations since the dPoW process will be initiated immediately after a new block appears in the chain.

#### Update instructions (for testing)

1. Stop all coin daemons and add in appropriate `.conf` file the following line:
```
blocknotify=curl -s --url "http://127.0.0.1:7776" --data "{\"agent\":\"dpow\",\"method\":\"updatechaintip\",\"blockhash\":\"%s\",\"symbol\":\"COIN\"}"
```
where COIN is a coin ticker, i.e. in komodo.conf - should be KMD, in MARTY.conf should be MARTY, etc. Don't mix up coin tickers plz. As a result each `.conf` file (i.e. `KMD`, `LTC`, `DOC`, `MARTY`, ...) should contain `blocknotify` call.

2. Start all the daemons again.

3. Clone the iguana repository and checkout the `patch-updatechaintip` branch that contains the update:
```bash
git clone https://github.com/DeckerSU/dPoW --branch patch-updatechaintip --single-branch dPoW_test
```

4. Build iguana and copy needed files:
```bash
cd dPoW_test\iguana
./m_notary_build # or make clean ; make -j$(nproc -all)
# copy your `wp_7776` and `pubkey.txt` files here from main dPoW folder (!)
```

5. Start notarisations:
```bash
./m_notary_main # start notarisations in mainnet
```

#### Advanced blocknotify solution

Sometimes it is necessary to correct something in the sequence of blocknotify commands. However, if you want to make changes to the `.conf` files, you will need to restart the daemons. To avoid this, there is an advanced solution. You can create a `notify.sh` script with the following content, for example, in the `$HOME/scripts/notify.sh` directory:

```bash
#!/usr/bin/env bash

# $1 - coin, $2 - blockhash
curl -s --url "http://127.0.0.1:7776" --data "{\"agent\":\"dpow\",\"method\":\"updatechaintip\",\"blockhash\":\"$2\",\"symbol\":\"$1\"}"
```

Then, the blocknotify call in the `.conf` files will look like this:

```
blocknotify=/home/decker/scripts/notify.sh KMD %s
```

If you need to make changes to the commands sequence, you will no longer need to restart the daemons. You will just need to modify the content of the `notify.sh` script.
