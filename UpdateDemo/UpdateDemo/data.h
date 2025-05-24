
// Nombre del dispositivo
const char* nombre = "esp-device";

// Credenciales Wi-Fi
const char* ssid = "VisitasWIFI";             // Nombre de la red Wi-Fi (SSID)
const char* password = "caracteristicas7";     // Contraseña de la red Wi-Fi

// Página HTML para la actualización del firmware
String Pagina = R"====(
<form method='POST' action='/actualizar' enctype='multipart/form-data'>
  <input type='file' name='update'><br>
  <input type='submit' value='Actualizar'>
</form>
)====";

// Credenciales para la autenticación en la interfaz web
const char* web_username = "admin";     // Nombre de usuario para acceder a la interfaz web
const char* web_password = "puertas";   // Contraseña para acceder a la interfaz web
