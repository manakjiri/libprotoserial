
**terminologie**

- hraniční fragment = první nebo poslední fragment transferru
- hlavní fragment = fragment transferu, který není hraniční
- řídicí fragment = ACK, REQ, atd. Nenese data, je to jen hlavička

# měření RTT
1. první (poslední) fragment předán interface 
2. od interface přijde konfirmace odeslání (tu interface generuje ve chvíli odeslání prvního byte)
3. od protějšku přijde ACK fragment (nebo v případě posledního potenciálně ještě REQ) - ACK i REQ jsou malé prioritní fragmenty, které by měly obcházet omezení na rate, protože by neměly významně přispívat k provozu

# řízení provozu
  - po každém kroku se změní stav transferu aby reflektoval akci. Tímto se přidává omezující podmínka na transfer - žádný transfer nesmí mít více fragmentů "ve vzduchu"
  - u hraničních fragmentů se čeká na odpověď, odesílání hlavních je řízeno peer.tx_rate a (?) potenciálními STATUS fragmenty. Hlavní fragmenty nejsou jednotlivě potvrzovány a vše je jištěno timouty. 
  - pokud se od konfirmace odeslání hraničního fragmentu překročí maximální mez RTT, musí se zahodit aktuální měření RTT a provést retransmit naposled odeslaného fragmentu.
  - pokud víc těchto pokusů selže a je překročen inactivity timeout, celý transfer se zahazuje, stav peer něco ve smyslu (unreachable += 1) (TODO)

# kroky odeslání transferu
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

# kroky přijímaní transferu
1. přijat první fragment (bude vždy první v pořadí, jelikož odesílatel čeká na ACK od nás), vytvoříme nový transfer a odešleme ACK, stav NEXT
2. jakýkoliv další fragment je zařazen, stav NEXT, pro poslední fragment odesláno ACK a stav DONE, generován receive_event a zahozena datová část
3. stav NEXT dlouho bez aktivity, 
4. stav DONE a expirace drop period, zahozen celý transfer (držíme pro případ ztráty posledního)


