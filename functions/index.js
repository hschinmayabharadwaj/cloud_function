const { onDocumentCreated } = require("firebase-functions/v2/firestore");
const logger = require("firebase-functions/logger");
const admin = require("firebase-admin");
const axios = require("axios");

admin.initializeApp();

exports.onFirestoreCommandCreate = onDocumentCreated(
  {
    document: "commands/{commandId}",
    region: "us-central1",
    maxInstances: 10,
  },
  async (event) => {
    const commandId = event.params.commandId;
    const data = event.data.data();

    if (!data) return null;

    logger.info("New command created", { commandId, data });

    // Extract required fields
    const espHost = data.espHost;           // ngrok URL or IP of ESP
    const cmd = data.action;                // TURN_ON, TURN_OFF, etc.
    const key = data.key;                   // your secret key
    const duration = data.duration || 1000; // optional duration in ms

    if (!espHost || !cmd || !key) {
      logger.error("Missing espHost, action, or key", { espHost, cmd, key });
      await event.data.ref.update({
        status: "failed",
        error: "Missing espHost, action, or key",
        processed: false,
      });
      return null;
    }

    try {
      // Build URL
      const url = `${espHost}/command?cmd=${cmd}&key=${key}&duration=${duration}`;

      // Send HTTP GET with ngrok header
      const response = await axios.get(url, {
        timeout: 5000,
        headers: {
          "ngrok-skip-browser-warning": "true", // important for free ngrok
          "User-Agent": "ESP8266-CloudFunction"
        },
      });

      logger.info("Command sent to ESP8266", {
        url,
        status: response.status,
        data: response.data,
      });

      await event.data.ref.update({
        processed: true,
        processedAt: admin.firestore.FieldValue.serverTimestamp(),
        status: "done",
      });
    } catch (err) {
      logger.error("Failed to send command to ESP8266", err);

      await event.data.ref.update({
        status: "failed",
        error: err.message,
        processed: false,
      });
    }

    return null;
  }
);
