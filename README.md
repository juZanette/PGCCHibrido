# Processamento Gráfico 2026/1

Este repositório contém códigos da disciplina de **Processamento Gráfico** do curso Ciência da Computação da Unisinos.

### ⚙️ **Informações Técnicas:**
- **API:** OpenGL  
- **Version:** 3.3+ (ou superior compatível com sua máquina)  
- **Profile:** Core  
- **Language:** C/C++  

Siga as instruções detalhadas em [GettingStarted.md](GettingStarted.md) para configurar e compilar o projeto.

## 📂 Estrutura do Repositório

```plaintext
📂 PGCCHibrido/
├── 📂 include/               # Cabeçalhos e bibliotecas de terceiros
│   ├── 📂 glad/              # Cabeçalhos da GLAD (OpenGL Loader)
│   │   ├── glad.h
│   │   ├── 📂 KHR/           # Diretório com cabeçalhos da Khronos (GLAD)
│   │       ├── khrplatform.h
├── 📂 common/                # Código reutilizável entre os projetos
│   ├── glad.c                 
├── 📂 src/                   # Código-fonte dos exercícios
│   ├── 📂 Exercicios/     
│   │   ├── 📂 M1/                
│   │       └── M1.cpp
│   │       └── README.md
│   │       └── Enunciado.pdf
│   │   ├── 📂 M2/                
│   │       └── M2.cpp
│   │       └── README.md
│   │       └── Enunciado.pdf
│   └── ...                  
├── 📂 build/                 # Diretório gerado pelo CMake (não incluído no repositório)
├── 📄 CMakeLists.txt         # Configuração do CMake para compilar os projetos
├── 📄 README.md              # Este arquivo, com a documentação do repositório
├── 📄 GettingStarted.md      # Tutorial detalhado sobre como compilar usando o CMake
├── 📄 ...
```
