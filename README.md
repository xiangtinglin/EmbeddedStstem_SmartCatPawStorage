# EmbeddedStstem_SmartCatPawStorage

[Demo]

https://www.youtube.com/watch?v=KiXPuzx5egM&list=PLXXc_QqkqR-U0l1N3cX_IKwmcRZHqCvCu

[Details]
1. 產品設計介紹：30s
2. 功能:

   （1）元件:
   
   sensor感重量 >> 邏輯條件判斷決定是啟動Motor、LED、Buzzer >> 結果會呈現在LCD螢幕上。
   
   （2）模式設計-五大 Status:
   
   - Status:Idle
     
   - Status:Action
     
     輕量模式:小於 15g 且 大於等於100g 👉 推向左。
     
     中等模式:大於 100g 且 小於等於1.3kg 👉推向右。
     
   - Status:Alert
     
     超重模式:大於 1.3kg 👉馬達不動 + 蜂鳴器警示 + LED燈亮。
     
   - Status:
     
     Sleep Mode休眠: 進入Status:Idle階段,15秒後進入休眠 >> 休眠後5秒螢幕自動熄滅（待機）。
     
     Waking重啟: 放置物品時自動喚醒。
     
4. 特色:
   
   （1）5V2A外接行動電源:同一電源可同時分別供應板子跟伺服馬達、電子元件所需。
   
   （2）雙臂伺服馬達。

   （3）休眠/重啟模式15s,再5秒後螢幕熄燈。
   
   （4）穩定設計:
   
   - 抗擾動:重量穩定2秒後才動作。
     
   - 閾值:+-50g,去抖動等等。
   
9. 原創性、自製部分多: ex.烙鐵、自製USB供電轉貼頭、道具,等等。
10. 實測: 4項物品、休眠機制、測試各種情境、各項指標是否正確，等等。
