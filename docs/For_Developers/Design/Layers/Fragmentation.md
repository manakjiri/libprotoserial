
# Fragmentation

## Initial thoughts

We can build the fragment fragmentation logic on top of the interface, this logic should have its own internal buffer for fragments received from events, because once the event fires, the fragment is forgotten on the interface side to avoid the need for direct access to the interface's RX queue.

fragmentation handler needs to implement basic congestion and flow control. each handler manages only one interface, each interface can be connected to multiple different interfaces with different addresses, so it would be beneficial to broadcast the state information to share it with everybody, but in terms of total overhead this may not be the best solution (since we are sending data that aims to prevent congestion...)

in order for this to be useful, the information needs to be up-to-date, so perhaps the most logical solution would be to embed it into the data itself. Perhaps a single additional byte in the fragmentation header might be sufficient to convey enough information about the receiver's state.

handler should store the following information about peers - current holdoff times and capacity estimates

the status value should reflect the receiver's available capacity either in absolute terms (which I do not find appealing) or using increase/decrease signaling - there will be other factors that collectively influence the rate of transmition. Reason why I think absolute would not work is because these systems are not necessarily linear.




**terminologie**

- hraniční fragment = první nebo poslední fragment transferru
- hlavní fragment = fragment transferu, který není hraniční
- řídicí fragment = ACK, REQ, atd. Nenese data, je to jen hlavička

## měření RTT
1. první (poslední) fragment předán interface 
2. od interface přijde konfirmace odeslání (tu interface generuje ve chvíli odeslání prvního byte)
3. od protějšku přijde ACK fragment (nebo v případě posledního potenciálně ještě REQ) - ACK i REQ jsou malé prioritní fragmenty, které by měly obcházet omezení na rate, protože by neměly významně přispívat k provozu

## řízení provozu
  - po každém kroku se změní stav transferu aby reflektoval akci. Tímto se přidává omezující podmínka na transfer - žádný transfer nesmí mít více fragmentů "ve vzduchu"
  - u hraničních fragmentů se čeká na odpověď, odesílání hlavních je řízeno peer.tx_rate a (?) potenciálními STATUS fragmenty. Hlavní fragmenty nejsou jednotlivě potvrzovány a vše je jištěno timouty. 
  - pokud se od konfirmace odeslání hraničního fragmentu překročí maximální mez RTT, musí se zahodit aktuální měření RTT a provést retransmit naposled odeslaného fragmentu.
  - pokud víc těchto pokusů selže a je překročen inactivity timeout, celý transfer se zahazuje, stav peer něco ve smyslu (unreachable += 1) (TODO)

## kroky odeslání transferu
1. transmit_transfer zařadila transfer do fronty, jeho stav je NEW, current = 0
2. stav NEW nebo NEXT, ale není RETRY, incrementuj current. V každém případě odešli current fragment, stav WAITING
3. stav WAITING 
  - přišla konfirmace odeslání, nastav stav na SENT a začni měřit čas
  - konfirmace selhala (interface odmítnul fragment), celý transfer se zahazuje
4. stav SENT
  - current je hraniční nebo retransmit_count > 0
  - přišel ACK fragment, ulož změřený rtt a nastav stav na NEXT nebo DONE, kde generujeme ack pro odesílatele a zahazujeme transfer
  - přišel REQ fragment, poslední je v tom případě v pořádku a chybí některý z hlavních, nastaven stav RETRY a current na požadovaný fragment, retransmit_count++
  - timeout přijetí ACK nebo REQ, nastav stav na RETRY (zde se může stát, že máme timeout moc striktní a ACK přijde o něco později, v takovém případě musíme prudce relaxovat tento timeout)
  - je hlavní a uplynulo dostatek času od odeslání, stav NEXT

## kroky přijímaní transferu
1. přijat první fragment (bude vždy první v pořadí, jelikož odesílatel čeká na ACK od nás), vytvoříme nový transfer a odešleme ACK, stav NEXT
2. jakýkoliv další fragment je zařazen, stav NEXT, pro poslední fragment odesláno ACK a stav DONE, generován receive_event a zahozena datová část
3. stav NEXT dlouho bez aktivity, 
4. stav DONE a expirace drop period, zahozen celý transfer (držíme pro případ ztráty posledního)


## transmit priority ordering

the previous implementation suffered from a load balancing problem, where it treated transfers completely independently. We need a way to order currently outgoing transfers withing the fragmentation handler in terms of "transmit priority". This should be a single metric that gives priority to certain transfers while pushing back others to prevent interface overload. 

Metrics that should play a role
1. how close is this transfer to completion - we need to prioritize transfers that are almost finished and hold back transfers that haven't gone out yet
  - the handler should attempt to minimize the number of transfers in progress (states NEXT, RETRY, WAITING and SENT)
2. transfers that fit into a single fragment
  - these carry little overhead - just one transmit and one ACK if everything goes smoothly
  - it is also likely that small transfers carry timely data as opposed by the long ones
3. time since the last transmit
  - we need a push for transfers that are otherwise low priority (ie. long ones near the beginning)

