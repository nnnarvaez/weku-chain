This instructions do not apply but are a good start for a how-to.


# Execute the following instructions for witness.
1. unregister as a witness (see note below)
2. modify/add `max-block-age = 2000000` in `config.ini`
3. stop witness and restart
4. register as a witness (see note below)

# How to register as a witness?

### Open cli_wallet and unlock it.
Then issue the enabling command
```
update_witness "Your-Witness-Name" "https://main.weku.io/@alexey05" WKA6XT9cBahMxC6t3PunG3jWkPrHWiCV1cHLyuCsLjahav14L5v2h {"account_creation_fee”:"1.000 WEKU", "maximum_block_size":131072,"sbd_interest_rate":0} true
```

replace _Your-Witness-Name_ with your account name, and replace the _public key_ with your public key above is the register command in cli_wallet

# How to unregister as a witness?
### Open cli_wallet and unlock it.
Then issue the disabling command

```
update_witness "Your-Witness-Name" "https://main.weku.io/@alexey05"  WKA1111111111111111111111111111111114T1Anm  {"account_creation_fee”:"1.000 WEKU", "maximum_block_size":131072,"sbd_interest_rate":0} true
```
replace _Your-Witness-Name_ with your account name in cli_wallet

# Note:
Minor check: 

* After unregistering, before stop the chain, wait around 5 seconds, and in your cli_wallet, 
run command: `get_active_witnesses`, check the return witness lists, make sure your name is not there then you can safely stop the chain

* After registering, run the same command to check your name is in the list.
