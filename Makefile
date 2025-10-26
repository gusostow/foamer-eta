# PlatformIO Makefile for foamer-display
# ESP32-S3 MatrixPortal board

PROJECT_DIR = firmware/foamer-display
PIO = platformio

# Docker/ECR variables
IMAGE_NAME = foamer-svc
IMAGE_TAG ?= latest
ENV ?= dev
AWS_REGION ?= us-east-1
AWS_ACCOUNT_ID = $(shell aws sts get-caller-identity --query Account --output text)
ECR_REPOSITORY = $(AWS_ACCOUNT_ID).dkr.ecr.$(AWS_REGION).amazonaws.com/foamer-$(ENV)-svc

.PHONY: compile upload monitor clean compiledb build-image push-image ecr-login

compile:
	cd $(PROJECT_DIR) && $(PIO) run

upload:
	cd $(PROJECT_DIR) && $(PIO) run -t upload

monitor:
	cd $(PROJECT_DIR) && $(PIO) device monitor

upload-monitor:
	cd $(PROJECT_DIR) && $(PIO) run -t upload -t monitor

clean:
	cd $(PROJECT_DIR) && $(PIO) run -t clean

compiledb:
	cd $(PROJECT_DIR) && $(PIO) run -t compiledb

# Docker targets
build-image:
	docker build -t $(IMAGE_NAME):$(IMAGE_TAG) .

ecr-login:
	aws ecr get-login-password --region $(AWS_REGION) | docker login --username AWS --password-stdin $(AWS_ACCOUNT_ID).dkr.ecr.$(AWS_REGION).amazonaws.com

push-image: build-image ecr-login
	docker tag $(IMAGE_NAME):$(IMAGE_TAG) $(ECR_REPOSITORY):$(IMAGE_TAG)
	docker push $(ECR_REPOSITORY):$(IMAGE_TAG)
