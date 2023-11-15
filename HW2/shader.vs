#version 330 core
struct Lightattr {
	vec3 Ia;
	vec3 Id;
	vec3 Is;
	vec3 dir;
	vec3 pos;
	float exp;
	float cutoff;
	float constant;
	float linear;
	float quadratic;
};
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;    
    float shininess;
}; 

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vertex_color;
out vec3 vertex_normal;
out vec3 vertex_pos;


uniform Lightattr lightattr[3];
uniform Material material;
uniform int pervertex;
uniform mat4 mvp;
uniform mat4 model;
uniform mat4 view;
uniform int lightmode;


vec3 direction(vec3 N, vec3 V){
	vec3 ambient = lightattr[0].Ia * material.ambient;

	//diffuse
	vec4 lightview = view * vec4(lightattr[0].pos,1.0);
	vec3 L = normalize(vec3(lightview.x,lightview.y,lightview.z)+V);
	vec3 H = normalize(L+V);
	float diff = dot(N , L);
	vec3 diffuse = lightattr[0].Id * ( material.diffuse * diff);

	//specular
	float spec = pow(max(dot(N , H), 0.0), material.shininess);
    vec3 specular = lightattr[0].Is * ( material.specular* spec ); 

	vec3 result = ambient + diffuse + specular;
	
	return result; 
}
vec3 point(vec3 N, vec3 V){
	vec3 ambient = lightattr[1].Ia * material.ambient;

	vec4 lightview = view * vec4(lightattr[1].pos,1.0);
	vec3 D = vec3(lightview.x, lightview.y,lightview.z) + V;
	vec3 L = normalize(vec3(lightview.x,lightview.y,lightview.z)+V);
	vec3 H = normalize(L + V);

	float dist = pow( max(pow(D.x,2)+pow(D.y,2)+pow(D.z,2),1),0.5);
	float att = min(1/(lightattr[1].constant + lightattr[1].linear * dist + lightattr[1].quadratic * pow(dist,2)),1);
	

	float diff = dot(N , L);
	vec3 diffuse = att * lightattr[1].Id * ( material.diffuse * diff);

	float spec = pow(max(dot(N , H), 0.0), material.shininess);
    vec3 specular = att * lightattr[1].Is * ( material.specular* spec ); 

	vec3 result = ambient + diffuse + specular;
	return result;
}
vec3 spot(vec3 N ,vec3 V){
	vec3 ambient = lightattr[2].Ia * material.ambient;

	vec4 lightview = view * vec4(lightattr[2].pos,1.0);
	vec3 D = vec3(lightview.x, lightview.y,lightview.z) + V;
	vec3 L = normalize(vec3(lightview.x,lightview.y,lightview.z)+V);
	vec3 H = normalize(L + V);


	float dist = pow( max(pow(D.x,2) + pow(D.y,2) + pow(D.z,2),1),0.5);
	float att = min(1/(lightattr[2].constant + lightattr[2].linear * dist + lightattr[2].quadratic * pow(dist,2)),1);

	vec4 dir = view * vec4(lightattr[2].dir,1.0);
	vec3 dirview = normalize(vec3(dir.x,dir.y,dir.z));
	float theta = dot(-L,dirview);
	float spoteffect = 0;
	float pi = 3.14159;
	
	if(theta > cos(radians(lightattr[2].cutoff )) )
		spoteffect = pow(max(theta,0),lightattr[2].exp);
	else
		spoteffect = 0;

	

	float diff = dot(N , L);
	vec3 diffuse = spoteffect *att * lightattr[2].Id * ( material.diffuse * diff);

	
	float spec = pow(max(dot(N , H), 0.0), material.shininess);
    vec3 specular = spoteffect * att * lightattr[2].Is * ( material.specular* spec ); 

	vec3 result = ambient + diffuse + specular;
	return result;

}
void main()
{
	// [TODO]
	vec4 Normal = transpose(inverse(model)) *  vec4(aNormal,0.0) ;
	vec4 viewPos =  model * vec4(aPos,1.0) ;

	gl_Position = mvp * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
	
	vec3 vPos = vec3(viewPos.x,viewPos.y,viewPos.z);
	
	vec3 N = normalize(vec3(Normal.x ,  Normal.y , Normal.z));
	vec3 V = -1*vec3(vPos);
	if(lightmode == 0)
		vertex_color = direction(N,V);
	else if(lightmode == 1 )
		vertex_color = point(N,V);
	else if(lightmode == 2 )
		vertex_color = spot(N,V);
	
	vertex_normal = N;
	vertex_pos = vPos;
}

