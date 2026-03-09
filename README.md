# Particle_Power_Outage_Alert
Vibe Coded notifications for detecting power outages with the Particle Boron404x board via ntfy.sh

⚡ Power Monitor: Complete Setup Guide
This guide covers the end-to-end setup for deploying a new Particle Boron power monitoring system. This system detects AC power loss, monitors LiPo battery health, checks cellular signal strength, and delivers interactive push notifications directly to your phone via ntfy.sh.

Phase 1: Hardware & Claiming
Before touching any code, the physical board needs to be connected to your Particle account.

Attach the Antenna: Carefully snap the cellular antenna into the u.FL connector on the Boron.

Connect Power: Plug the 3.7V LiPo battery into the JST connector. Then, plug the Boron into a reliable USB power source (this acts as your "AC Power").

Setup the device: http://setup.particle.io/

Claim the Device: Open the Particle mobile app on your phone, click the + icon, scan the data matrix sticker on the Boron, and follow the prompts to claim the device to your account.

Phase 2: Generating a Particle Access Token
Your Webhook needs permission to securely talk back to your Boron when you press the "Check Status" button on your phone. To do this, it needs an access token.

Open a Web browser: Go to [build.particle.io.] (https://docs.particle.io/reference/cloud-apis/access-tokens/?q=API#create-a-token-browser-based-)

Create your Token (Browser Based): Enter your Particle Credintials select Expires Never (or different depending on your use case). You will see an "Access Token" string. Copy this to your clipboard. (Note: Keep this secret!) 
Go to https://console.particle.io/secrets and Create New Secret. Paste the Access Token and select a name you will use to refer back to YOUR_ACTUAL_HARDCODED_TOKEN.

Phase 3: Setting Up the ntfy Webhook
This tells the Particle Cloud how to format your alerts and where to send them.

Navigate to Integrations: Go to the Particle Console -> Integrations -> New Integration -> Webhook.

Switch to Custom: Click the Custom Template tab.

Paste the Template: Paste your fully finished JSON webhook code here.

Make Two Crucial Edits:

Find "topic": "my_unique_topic_subscription" and change it to a secure, random 6-word phrase using hyphens (e.g., "topic": "purple-dinosaur-battery-shed-monitor-alert"). This acts as your private password.

Find "Authorization": "Bearer YOUR_ACTUAL_HARDCODED_TOKEN" and replace YOUR_ACTUAL_HARDCODED_TOKEN with the access token you copied in Phase 2.

Save: Click Create Webhook.

Phase 4: Flashing the Firmware
Now we give the Boron its brain.

Open the Code Editor: Go back to the Particle Web IDE (or VS Code / Workbench).

Paste the Code: Paste your finalized C++ firmware into the main .ino file.

Target and Flash: Click the Target icon on the left, select your newly claimed Boron, and click the lightning bolt Flash icon. Wait for the device to restart and breathe cyan.

Phase 5: Initializing the Device Memory
The Boron has onboard EEPROM memory to remember its own name, but it needs you to tell it what that name is on the very first boot.

Name the Device: In the Particle Console, go to Devices, select your Boron, click the edit pencil next to its name, and call it something recognizable (e.g., "Shed Monitor").

Sync the Memory: On that exact same page, look at the right-hand side under Functions. You will see the syncName button we created. Leave the argument box blank and click Call.

Verify: You should see a "DEBUG" event pop up in your console stream saying "New name saved to EEPROM!".

Phase 6: Mobile App Setup
Finally, let's connect your phone so you can actually see the alerts.

Download ntfy: Install the ntfy app from the iOS App Store or Google Play Store.

Subscribe: Open the app, tap the + icon to add a new subscription.

Enter Your Topic: Type in the exact same 6-word secure phrase you put in the Webhook (e.g., purple-dinosaur-battery-shed-monitor-alert).

Test the System: Unplug the Boron from USB power. Within a few seconds, your phone should light up with a Priority 5 Critical Alert! Tap the "Check Status" button on the notification to test your two-way communication.
