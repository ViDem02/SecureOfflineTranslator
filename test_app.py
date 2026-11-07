import unittest
from unittest.mock import patch

import app


class AppTests(unittest.TestCase):
    def test_loopback_endpoint_is_required(self):
        self.assertTrue(app.is_loopback_url("http://127.0.0.1:11434/api/generate"))
        self.assertFalse(app.is_loopback_url("https://example.com/api/generate"))

    def test_technical_tokens_are_restored(self):
        protected, tokens = app.preserve_technical_tokens("Set 12 mm and inspect ISO-9001")
        self.assertNotIn("12 mm", protected)
        self.assertEqual(app.restore_technical_tokens(protected, tokens), "Set 12 mm and inspect ISO-9001")

    @patch.object(app, "external_network_reachable", return_value=False)
    @patch.object(app, "is_loopback_url", return_value=True)
    @patch("app.urllib.request.urlopen")
    def test_translation_uses_local_model(self, urlopen, _, __):
        response = urlopen.return_value.__enter__.return_value
        response.__iter__.return_value = iter([b'{"response":"Hello 12 mm","done":true}\n'])
        self.assertEqual(app.translate("Ciao 12 mm", "Italian", "English"), "Hello 12 mm")


if __name__ == "__main__":
    unittest.main()
