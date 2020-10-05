#!/usr/bin/env python3

from flask import (
    Flask,
    Response,
    render_template,
    redirect,
    url_for,
    flash,
    abort,
    request,
)
from flask_ldap3_login import LDAP3LoginManager
from ldap3 import ALL_ATTRIBUTES
from flask_ldap3_login.forms import LDAPLoginForm
from flask_login import (
    LoginManager,
    login_required,
    login_user,
    logout_user,
    UserMixin,
    current_user,
)
from flask_caching import Cache
from flask_wtf import FlaskForm
from wtforms import StringField, SubmitField, PasswordField, validators, ValidationError
from wtforms.fields.html5 import EmailField
from urllib.parse import urlparse, urljoin
from celery import Celery
from ldap3 import HASHED_SALTED_SHA, ObjectDef, Reader
from ldap3.utils.hashed import hashed
from flask_mail import Mail, Message

import os
import default_settings
import secrets


app = Flask(__name__)
app.config.from_object("chall.default_settings")
if "APP_SETTINGS" in os.environ:
    app.config.from_envvar("APP_SETTINGS")

app.secret_key = app.config["SECRET_KEY"]

login_manager = LoginManager(app)
login_manager.login_view = "login"

ldap_manager = LDAP3LoginManager(app)

cache = Cache(app)

mail = Mail(app)


def make_celery(app):
    celery = Celery(
        app.import_name,
        backend=app.config["CELERY_RESULT_BACKEND"],
        broker=app.config["CELERY_BROKER_URL"],
    )
    celery.conf.update(app.config)

    class ContextTask(celery.Task):
        def __call__(self, *args, **kwargs):
            with app.app_context():
                return self.run(*args, **kwargs)

    celery.Task = ContextTask
    return celery


celery = make_celery(app)


class User(UserMixin):
    def __init__(self, dn, username, data):
        self.dn = dn
        self.username = username
        self.data = data

    def __repr__(self):
        return self.dn

    def get_id(self):
        return self.dn


def is_safe_url(target):
    ref_url = urlparse(request.host_url)
    test_url = urlparse(urljoin(request.host_url, target))
    return test_url.scheme in ("http", "https") and ref_url.netloc == test_url.netloc


@login_manager.user_loader
def load_user(id):
    return cache.get("user_{}".format(id))


@ldap_manager.save_user
def save_user(dn, username, data, memberships):
    user = User(dn, username, data)
    cache.set("user_{}".format(user), user)
    return user


@app.route("/login", methods=["GET", "POST"])
def login():
    form = LDAPLoginForm()

    if form.validate_on_submit():
        login_user(form.user)
        flash("Logged in successfully.")

        next = request.args.get("next")
        if not is_safe_url(next):
            return abort(400)

        return redirect(next or url_for("index"))

    return render_template("login.html", form=form)


@celery.task
def do_password_reset(username, password):
    try:
        ldif = f"""
dn: uid={username},{ldap_manager.full_user_search_dn}
changeType: modify
replace: userPassword
userPassword: {password}
"""
        from subprocess import Popen, PIPE

        ldapmodify = Popen(
            [
                "ldapmodify",
                "-x",
                "-D",
                app.config["LDAP_BIND_USER_DN"],
                "-w",
                app.config["LDAP_BIND_USER_PASSWORD"],
                "-H",
                f"ldap://{ app.config['LDAP_HOST'] }",
            ],
            stdin=PIPE,
            stdout=PIPE,
            stderr=PIPE,
        )
        ldapmodify.stdin.write(ldif.encode())
        stdout, stderr = ldapmodify.communicate()
        return (stdout.decode(), stderr.decode())
    except:
        pass


@app.route("/reset/<string:token>", methods=["GET", "POST"])
def reset_password(token):
    class ResetPasswordForm(FlaskForm):
        password = PasswordField()
        submit = SubmitField()

    uid = cache.get("reset_{}".format(token))
    if not uid:
        flash("The reset token you have provided is invalid")
        abort(403)

    form = ResetPasswordForm()

    if form.validate_on_submit():
        flash("Your password is being reset!")

        cache.delete("reset_{}".format(token))
        result = do_password_reset.apply_async([uid, form.password.data])
        stderr, stdout = result.wait()

        flash(stderr)
        flash(stdout)

        next = request.args.get("next")
        if not is_safe_url(next):
            return abort(400)

        return redirect(next or url_for("index"))

    return render_template("reset_password.html", form=form, uid=uid)


@celery.task
def send_reset_mail(username):
    try:
        account = ObjectDef(["account", "mailboxRelatedObject"])
        account += ["uid", "mail"]
        query = "uid: {}".format(username)
        reader = Reader(
            ldap_manager.connection, account, ldap_manager.full_user_search_dn, query
        )
        reader.search()

        if not reader.entries:
            return

        user = reader.entries[0]

        token = secrets.token_urlsafe(app.config["RESET_TOKEN_LENGTH"])
        cache.set("reset_{}".format(token), username)

        msg = Message(
            recipients=[str(user.mail)],
            subject="Your password reset link",
            sender=app.config["MAIL_SENDER_ADDRESS"],
        )

        msg.body = render_template("forgot_password.txt", token=token)
        print(msg)
        mail.send(msg)
    except:
        pass


@app.route("/reset", methods=["GET", "POST"])
def request_reset():
    class ResetPasswordRequestForm(FlaskForm):
        username = StringField(
            "username",
            [
                validators.Regexp(
                    "^\w+$",
                    message="Username must contain only letters numbers or underscore",
                ),
                validators.Length(
                    min=5, max=25, message="Username must be betwen 5 & 25 characters"
                ),
            ],
        )
        submit = SubmitField()

    form = ResetPasswordRequestForm()
    if form.validate_on_submit():
        send_reset_mail.apply_async([form.username.data])
        flash("A password reset mail has been sent if your user exists!")

        next = request.args.get("next")
        if not is_safe_url(next):
            return abort(400)

        return redirect(next or url_for("index"))

    return render_template("reset_request.html", form=form)


@celery.task
def create_user(username, email, password):
    try:
        ldap_manager.connection.add(
            "uid={username},{dn}".format(
                username=username,
                dn=ldap_manager.full_user_search_dn,
            ),
            object_class=["account", "simpleSecurityObject", "mailboxRelatedObject", "top"],
            attributes=dict(
                uid=username,
                mail=email,
                userPassword=hashed(HASHED_SALTED_SHA, password),
            ),
        )
        msg = Message(
            recipients=[email],
            subject="Your account has been created!",
            sender=app.config["MAIL_SENDER_ADDRESS"],
        )

        msg.body = render_template("registration.txt", username=username)
        print(msg)
        mail.send(msg)
    except:
        pass

@app.route("/register", methods=["GET", "POST"])
def register():
    class RegisterForm(FlaskForm):
        username = StringField(
            "username",
            [
                validators.Regexp(
                    "^\w+$",
                    message="Username must contain only letters numbers or underscore",
                ),
                validators.Length(
                    min=5, max=25, message="Username must be betwen 5 & 25 characters"
                ),
            ],
        )
        email = EmailField()
        password = PasswordField()
        submit = SubmitField()

    form = RegisterForm()
    if form.validate_on_submit():
        create_user.apply_async(
            [form.username.data, form.email.data, form.password.data]
        )
        flash("Your account should be created soon!")

        next = request.args.get("next")
        if not is_safe_url(next):
            return abort(400)

        return redirect(next or url_for("index"))

    return render_template("register.html", form=form)


@login_required
@app.route("/logout")
def logout():
    logout_user()
    return redirect("/")


@app.route("/download")
@login_required
def download():
    ldap_manager.connection.search(
        current_user.dn, "(objectClass=*)", attributes=ALL_ATTRIBUTES
    )
    return Response(ldap_manager.connection.response_to_ldif(), mimetype="text/plain")


@app.route("/privacy")
def privacy():
    return render_template("privacy.html")


@app.route("/")
@login_required
def index():
    return render_template("index.html")


if __name__ == "__main__":
    app.run()
