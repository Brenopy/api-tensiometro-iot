import os
from datetime import datetime
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
from pymongo import MongoClient
from dotenv import load_dotenv

load_dotenv()

app = FastAPI(
    title="API Tensiômetro IoT",
    description="Endpoint para recebimento de dados do solo via ESP32"
)

# Conexão com o MongoDB Atlas
MONGO_URI = os.getenv("MONGO_URI")

try:
    client = MongoClient(MONGO_URI)
    db = client["agrotech"]
    colecao = db["leituras_tensiometro"]
    client.admin.command('ping')
    print("Conectado ao MongoDB Atlas com sucesso!")
except Exception as e:
    print(f"Erro ao conectar no MongoDB: {e}")

# Schema de validação para as leituras do solo
class LeituraSolo(BaseModel):
    dispositivo_id: str = Field(default="tensiometro_01", example="tensiometro_01")
    pressao_kpa: float = Field(..., example=18.5)
    leitura_bruta: int = Field(..., example=841200)

@app.get("/")
def status_api():
    return {"status": "online", "mensagem": "API do Tensiômetro operando corretamente."}

@app.post("/api/leituras", status_code=201)
def salvar_leitura(dados: LeituraSolo):
    try:
        # Lógica para categorizar o estado da umidade do solo
        status_solo = "Solo Úmido"
        if dados.pressao_kpa > 30.0:
            status_solo = "Solo Seco / Irrigar"
        elif dados.pressao_kpa > 10.0:
            status_solo = "Umidade Ideal"

        documento = {
            "dispositivo_id": dados.dispositivo_id,
            "pressao_kpa": dados.pressao_kpa,
            "leitura_bruta": dados.leitura_bruta,
            "status_solo": status_solo,
            "data_hora": datetime.utcnow()
        }

        # Insere no MongoDB Atlas
        resultado = colecao.insert_one(documento)

        return {
            "sucesso": True,
            "id_registro": str(resultado.inserted_id),
            "status_solo": status_solo
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erro ao salvar leitura: {str(e)}")