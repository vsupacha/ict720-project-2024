# Python runtime
FROM python:3.10

# Working directory
WORKDIR /code

# copy required libraries
COPY ./requirements.txt /code/requirements.txt

# install required libraries
RUN pip install --no-cache-dir --upgrade -r /code/requirements.txt

# copy all files
COPY . /code

# run code
CMD uvicorn api_app:app --host 0.0.0.0 --port ${DEV_API_PORT}