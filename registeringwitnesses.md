Once wekud is built you need to declare your witness intention and broadcast it on the platform


# How to register as a witness?

### Open cli_wallet and unlock it.
Then issue the enabling command
```
update_witness "Your-Witness-Name" "https://main.weku.io/@alexey05" WKA6XT9cBahMxC6t3PunG3jWkPrHWiCV1cHLyuCsLjahav14L5v2h {"account_creation_fee”:"1.000 WEKU", "maximum_block_size":65536,"sbd_interest_rate":1000} true
```

replace _Your-Witness-Name_ with your account name, and replace the _public key_ with your public key above is the register command in cli_wallet

# How to unregister as a witness?
### Open cli_wallet and unlock it.
Then issue the disabling command

```
update_witness "Your-Witness-Name" "https://main.weku.io/@alexey05"  WKA1111111111111111111111111111111114T1Anm  {"account_creation_fee”:"1.000 WEKU", "maximum_block_size":65536,"sbd_interest_rate":1000} true
```
replace _Your-Witness-Name_ with your account name in cli_wallet

# Note:
Minor check: 

* After unregistering, before stop the chain, wait around 5 seconds, and in your cli_wallet, 
run command: `get_active_witnesses`, check the return witness lists, make sure your name is not there then you can safely stop the chain

* After registering, run the same command to check your name is in the list.
