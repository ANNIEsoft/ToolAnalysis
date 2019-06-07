//----------Author's Name:F-X Gentit
//----------Copyright:Those valid for CNRS sofware
//----------Modified:20/8/2003
#include "Riostream.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TF1.h"
#include "TDatime.h"
#include "TPoly3.h"
#include "TOnePadDisplay.h"
#include "TBandedLE.h"
#include "TSplineFit.h"
#include "TMath.h"

R__EXTERN TOnePadDisplay *gOneDisplay;
TSplineFit *gSplineFit = 0;

Int_t       TSplineFit::fgNextDraw       = 0;
Int_t       TSplineFit::fgM              = 6;
Int_t       TSplineFit::fgU              = 7;
TArrayI    *TSplineFit::fgCat            = 0;
TObjArray  *TSplineFit::fgFits           = 0;
TString    *TSplineFit::fgProgName       = 0;
TString    *TSplineFit::fgWebAddress     = 0;
TString    *TSplineFit::fgFileName       = 0;
TFile      *TSplineFit::fgFitFile        = 0;
TTree      *TSplineFit::fgFitTree        = 0;
TBranch    *TSplineFit::fgFitBranch      = 0;
Int_t       TSplineFit::fgNChanRand      = 128;
Int_t       TSplineFit::fgCounter        = 0;

//ClassImp(TSplineFit)
//______________________________________________________________________________
//
//
//BEGIN_HTML <!--
/* -->
</pre>
		<div align="center">
			<h1>TSplineFit</h1>
			<h1>General Tool for handling variable parameters</h1>
			<h2></h2>
			<h3>Introduction</h3>
			<div align="left">
				<p><b>SplineFit is much more than a tool allowing fits of splines. It is conceived

as a general solution for the handling of variable parameters inside an application.&nbsp;</b>Let

us take the example of a Monte-Carlo of optical photons, like <a href="http://gentit.home.cern.ch/gentit/">Litrani</a>.
In a Monte-Carlo like Litrani, variable parameters may be: index of refraction, absorption
length, diffusion length, which vary according to wavelength. Or gain profile of
APD which varies according to distance in <font face="Symbol">m</font>m, and so on
For each of these variable quantities, one has generally at hand a series of measurements,
for instance taken from the &quot;<i>Handbook of Optical Constants</i>&quot;.&nbsp;The
problem is to go from these measurements
towards a method for getting a fitted value for any of the values of the parameter

like wavelength or depth or whatsoever. <b>SplineFit</b>
is the general solution for handling this kind of problem.&nbsp;One inserts the measurements


into the constructor of <b>TSplineFit</b>, and the fitting splines are found, allowing


for a given fit <code><b>pf</b></code> to call <b><code>y = pf-&gt;V(x)</code></b>;

giving the value <b>y</b> for the input parameter <b>x</b>.</p>
			</div>
			<div align="center">

			</div>
			<div align="left">
				<p><b>But we ask much more to a general tool for variable parameters. The requirements

are the following:</b></p>
				<ul>
					<li><b>SplineFit</b> must cope with any kind of structure.&nbsp;This is where

fits of spline are much more interesting than for instance fits of polynoms. It is

a well known fact that fits of polynoms fail when tried on a complicated structure.&nbsp;Increasing

the degree of the fit lead to bigger matrices, whose inversion very soon fails. It

is why polynomial fits in <a href="http://root.cern.ch/"><b><code>Root</code></b></a>
are limited to the 9th degree. Spline fits also lead to matrix inversion, but here
the matrix has the very nice property that it is a band matrix. Spline fits with
a band matrix of size up to 4000 do succeed! In the old Litrani, where polynomial
fits were used, we had to replace fits with interpolation each time the polynomial
fit failed.&nbsp;This was an annoyance since an interpolation instead of a fit does
not smooth anymore the statistical fluctuations or errors on measurements.					<li><b>SplineFit</b>
is also able of spline interpolations or linear interpolations.&nbsp;There
are (rare) cases where interpolations have to be preferred.
					<li><b>It should be possible to impose lower or upper bound to fits</b>. Take

for example histograms where bin content below 0 is meaningless, or fit of index

of refraction, where values below 1 are meaningless.
					<li><b>Fits should contain the information of where the measurements for the
fit were taken</b>.
					<li><b>Fits should contain the information of what is plotted in function of

what parameter</b>.
					<li><b>The chi2 of the fit must be available</b>.
					<li><b>It should be possible to generate random numbers according to the distribution

of the fit</b>.
					<li><b>Variable parameters should offer the possibility to be stored into different

categories</b>.&nbsp;Take the example quoted above for the use in Litrani: index

of refraction, absorption length, diffusion length, and so on.
					<li><b>Inside a given category, when fits are identical up to only one differing

parameter, fits are grouped inside a family</b>. This offers in particular the possibility

of interpolation between the results for two different fits belonging to the same

family.
					<li><b>Inside a program, all the variable parameters represented by fits can

be stored into an ordered collection</b>, and are retrievable by name.
					<li><b>Once defined, a variable parameter represented by a fit can be stored

into a database</b>, so that it can be retrieved by all different programs using

it. Its definition in the form of <b><a href="http://root.cern.ch/root/Cint.html"><code>CINT</code></a></b>

code is used only once, and no more needed when the fit is stored into the database.
					<li><b>At last, nice possibilities of drawing the fit should be provided</b>.
				</ul>
				<p><b>SplineFit cope with all these requirements</b>.&nbsp;We will develop all

of them below.&nbsp;But first a few words on the software language and system used

and on the mathematics behind <b>SplineFit</b>.</p>
				<p></p>
			</div>
			<div align="center">
				<h3>Language, system and mathematics.</h3>
			</div>
			<div align="left">
				<p></p>
				<p><b>SplineFit</b> is written in C++ and constructed upon <a href="http://root.cern.ch/"><b><code>Root</code></b></a>,


so that it is available only for programs working within the framework of <b><code>Root</code></b>,


and is driven by <b><a href="http://root.cern.ch/root/Cint.html"><code>CINT</code></a></b>


code. It uses an other class, <b><a href="TBandedLE.html"><code>TBandedLE</code></a></b>,


which is my C++ translation of the <b>F406 DBEQN</b> program of the <b><a href="http://user.web.cern.ch/user/cern.html"><code>CERN</code></a></b>

packlib library, originally written by G.A.Erskine. <b>BandedLE</b> stands for Banded

Linear
Equations. Class <b><code>TBandedLE</code></b> solves a system of <b>N</b> simultaneous


linear equations with <b>k</b> right-hand sides, the coefficient matrix being a band


matrix with bandwidth <b><code>2*m+1</code>. </b>A complete description of the mathematics

behind spline fits is appended at the end of this page.</p>
				<p><b>SplineFit</b> uses 4 classes:</p>
			</div>
			<div align="center">
				<table border="10" cellpadding="4" cellspacing="4">
					<caption>
						<h4>The 4 classes of SplineFit</h4>
					</caption>
					<tr>
						<td><b>TSplineFit</b></td>
						<td>this class, the main one</td>
					</tr>
					<tr>
						<td><b><a href="TBandedLE.html">TBandedLE</a></b></td>
						<td>solution of linear systems with band matrix</td>
					</tr>
					<tr>
						<td><b><a href="TOnePadDisplay.html">TOnePadDisplay</a></b></td>
						<td>class used to show the fits</td>
					</tr>
					<tr>
						<td><b><a href="TZigZag.html">TZigZag</a></b></td>
						<td>allowing extension to 2D or 3D fits</td>
					</tr>
				</table>
			</div>
			<div align="left">
				<p></p>
				<p></p>
			</div>
			<div align="center">
				<h3>SplineFit as fit of splines.</h3>
			</div>
			<div align="left">
				<p>There are 9 different constructors of the class <b>TSplineFit</b>. Choice

of the right constructor allow to:</p>
				<ul>
					<li>choose among 1D, 2D or 3D fits
					<li>choose among spline fits or spline interpolation or linear interpolation
					<li>choose among entering data under the form of <b>real arrays</b>, <b><a href="http://root.cern.ch/root/html/TGraphErrors.html">TGraphErrors</a></b>,

or <b><a href="http://root.cern.ch/root/html/TH1D.html">histograms</a></b>.
				</ul>
				<p>The use of <b>SplineFit</b> for 2D or 3D fits is explained below.</p>
				<p>The call to the constructor finds the solution of the problem by calling <b><a href="TBandedLE.html">TBandedLE</a></b>,

except when spline interpolation is asked, where it simply calls the Root class <b><a href="http://root.cern.ch/root/html/TSpline3.html">TSpline3</a></b>,

or except when a simple linear interpolation is asked for, in which case nothing

has to be called for a solution. A very important information is returned when the

solution has been found: it is the quality of the solution which has been calculated

by a call to <code><b>TBandedLE::Verify</b>()</code>. Experience shows that when

this value is smaller than <b><code>10^-7</code></b>, everything is ok and the fit

is good.&nbsp;When the returned value is bigger, have a look at the display of the

fit.</p>
				<p>If you want to get the value of the fit at x, call <b>TSplineFit::V(x)</b>.

For 2D or 3D fits, call <b>TSplineFit::V(x,y)</b> or <b>TSplineFit::V(x,y,z)</b>.</p>
				<p></p>
				<p></p>
			</div>
			<div align="center">
				<h3>Storing useful informations with the fit.</h3>
			</div>
			<div align="left">
				<p>After the call to the constructor, it is highly recommended to store useful

informations inside the fit:</p>
			</div>
			<ul>
				<div align="left">
					<li>the <b>date</b> when the fit was produced is automatically stored
					<li>calling <b>SetSource()</b> allows you to store the information of where

you got the measurements from.
					<li>calling <b>SetMacro()</b> allows you to store the name of the <b><code>CINT</code></b>

macro having produced the fit.					<li>calling <b>SetXLabel()</b> [<b>SetYLabel()</b>

or <b>SetZLabel()</b> for 2D or 3D fits] allows you to store the information in function

of what quantity is the fit varying.&nbsp;For instance &quot;wavelength [nm]&quot;

for an index of refraction.
					<li>calling <b>SetVLabel()</b> stores the information of what is the variable

quantity, for example &quot;index of refraction&quot;
					<li>calling <b>SetDefaultLabels()</b> will add to the display of the fit the

3 following informations:
				</div>
				<ul>
					<div align="left">
						<li>label1 : source of measurements for this fit, given by SetSource()
						<li>label2 : name of the CINT macro having produced this fit, given by SetMacro()
						<li>label3 : &quot;SplineFit&quot; followed by the date of production of this

fit
					</div>
				</ul>
			</ul>
			<div align="left">
				<p>After the fit is totally defined, you can call method <b><code>Print()</code></b>

to have a full printing of all the fit characteristics.</p>
				<p></p>
				<div align="center">
					<div align="center">
						<h3>Bounds</h3>
					</div>
					<div align="left">
						<p>The possibility exists to impose a lower bound <b><code>ylow</code></b>

for the fitted value <b>y</b>. Look at the arguments <b><code>lowbounded</code></b>

and <b><code>ylow</code></b> of the constructor. This is interesting for instance

in case you fit measurements for which a negative value does not make sense. Or for

instance when fitting an index of refraction in function of wavelength, it would

not make sense to get a value smaller than 1. It is also particularly interesting

when fitting an histogram, because by definition histograms can only have positive

content. This is obtained by fitting, not the <code><b>y[i]</b></code> values, but

the <code><b>yn[i]</b></code> values, where:</p>
					</div>
					<div align="center">
						<p><code><b>yn[i] = y[i] - ylow - 1/(y[i] - ylow)</b></code></p>
					</div>
					<div align="left">
						<p>The reverse transformation is then applied when giving the result. Obviously,

the errors on <code><b>y[i]</b></code> are also transformed before the fit. If a

lower bound is requested and if one of the value <code><b>y[i]</b></code> is below

the bound, it will be modified and set slightly above the bound. A warning will be

issued. If one of the values <code><b>y[i]</b></code> is exactly equal to the bound,

the above transformation would bring it to minus infinity, which would prevent any

possibility of fit. So this value <code><b>y[i]</b></code> is set slightly above

the bound.&nbsp;Slightly means here <b><code>ylow + (ymax-ylow)*eps</code></b>, where

<b><code>ymax</code></b> is the biggest of all <code><b>y[i]</b></code>, and <b><code>eps</code></b>

has been set to <b><code>0.5*e-3</code></b>.&nbsp;The smallest <b><code>eps</code></b>

is chosen, the biggest the perturbation affecting the fit.&nbsp;Going towards 0 for

<b><code>eps</code></b> gives more and more &quot;agitated&quot; curves.&nbsp;The

value chosen for <b><code>eps</code></b> is a compromise.</p>
						<p>The possibility also exists to set an upper bound. See the arguments <b><code>highbounded</code></b>

and <b><code>yup</code></b>. But here the fit is <b>not</b> modified and <b>can</b>

give values above <b><code>yup</code></b>. It is only when calling method <b>V()</b>

that if the fitted value is above <b><code>yup</code></b> it is replaced by<b> <code>yup</code></b>!

Be aware of the difference between lower and upper bound.</p>
					</div>
				</div>
			</div>
			<div align="center">
				<h3>Fits or interpolation</h3>
			</div>
			<div align="left">
				<p>One of the arguments of the constructor of <b>TSplineFit</b> is the number

of measurements to take per spline (per sub-interval containing one spline). The

less this number of measurements, the greatest the number of splines to use, but

the closer to the measurements is the fit.&nbsp;You can play with this parameter

until the fit looks good for you or until the chi2 of the fit is close to 1. If you

happen to ask for only one measurement per spline, then <b>TSplineFit</b> understands

that you do not want a fit, but an interpolation.&nbsp;The machinery of<code> <b><a href="TBandedLE.html">TBandedLE</a></b></code>

is then <b>not</b> used.&nbsp;Instead,
the Root class <b><code><a href="http://root.cern.ch/root/html/TSpline3.html">TSpline3</a></code></b>


is called. In case of interpolation instead of fit, the provided values for the errors


on <code><b>y[i]</b></code> are irrelevant. Be aware also that some methods of <b>TSplineFit</b>,


like <b><code>MinMax()</code></b>, do not work in case of interpolation.</p>
				<p></p>
			</div>
			<div align="center">
				<h3>Name, title and category of fits</h3>
			</div>
			<div align="left">
				<p>The clas <b>TSplineFit</b> derives from <b><code><a href="http://root.cern.ch/root/html/TNamed.html">TNamed</a></code></b>,
so that each fit has a name and a title. A given fit may be retrieved from the collection
or from the file by its name. Fits are belonging to different categories. Look for
instance in the table below what are the different categories of fits used in Litrani.
If you intend to use TSplineFit independently of Litrani, you are free to define
names and titles of all categories.&nbsp;If you intend to use it also with Litrani,
please do not change names and titles for categories 1 to 15, as given in the table
below. And do not define names and titles for categories 16-20, which are reserved
for Litrani. You can freely use categories 21 to 99.</p>
				<p>Notice that caregory 0 is special: a fit of category 0 <b><font color="#b22222">cannot</font></b>
be put into the database file [see below the paragraph on the database file].&nbsp;The
reason for that is to allow users which are only interested in making a fit of splines
to use SplineFit without the database.</p>
				<p></p>
			</div>
			<table border="8" cellpadding="4" cellspacing="2" bgcolor="#fffff0">
				<caption>
					<h3>Example of use of categories in Litrani</h3>
				</caption>
				<tr>
					<td>
						<div align="center">
							<h3><font color="#b22222">Name</font></h3>
						</div>
					</td>
					<td>
						<div align="center">
							<h3><font color="#b22222">Title</font></h3>
						</div>
					</td>
					<td>
						<div align="center">
							<h3><font color="#b22222">Nb.</font></h3>
						</div>
					</td>
					<td>
						<div align="center">
							<h3><font color="#b22222">variation according</font></h3>
						</div>
					</td>
				</tr>
				<tr>
					<td>miscellaneous</td>
					<td>miscellaneous</td>
					<td>0</td>
					<td>anything</td>
				</tr>
				<tr>
					<td>RefrIndex</td>
					<td>Index of refraction</td>
					<td>1</td>
					<td>wavelength</td>
				</tr>
				<tr>
					<td>DielTensor</td>
					<td>Element of dielectric tensor</td>
					<td>2</td>
					<td>wavelength</td>
				</tr>
				<tr>
					<td>RIndexRev</td>
					<td>Real part of refraction index</td>
					<td>3</td>
					<td>wavelength</td>
				</tr>
				<tr>
					<td>IIndexRev</td>
					<td>Im.  part of refraction index</td>
					<td>4</td>
					<td>wavelength</td>
				</tr>
				<tr>
					<td>AbsorptionLength</td>
					<td>Absorption Length</td>
					<td>5</td>
					<td>wavelength</td>
				</tr>
				<tr>
					<td>AbsLengthTensor</td>
					<td>Element of absorption length tensor</td>
					<td>6</td>
					<td>wavelength</td>
				</tr>
				<tr>
					<td>DiffusionLength</td>
					<td>Diffusion Length</td>
					<td>7</td>
					<td>wavelength</td>
				</tr>
				<tr>
					<td>DiffLengthTensor</td>
					<td>Element of diffusion length tensor</td>
					<td>8</td>
					<td>wavelength</td>
				</tr>
				<tr>
					<td>DiffVersusReflec</td>
					<td>Proportion of diffusion versus reflection</td>
					<td>9</td>
					<td>%</td>
				</tr>
				<tr>
					<td>QuantumEff</td>
					<td>Quantum efficiency</td>
					<td>10</td>
					<td>0&lt;--&gt;1</td>
				</tr>
				<tr>
					<td>Dedx</td>
					<td>Energy deposited by crossing</td>
					<td>11</td>
					<td>- or energy</td>
				</tr>
				<tr>
					<td>AngularDistrib</td>
					<td>Angular distribution of photons</td>
					<td>12</td>
					<td>radian</td>
				</tr>
				<tr>
					<td>GainProfile</td>
					<td>Gain profile of APD or PIN</td>
					<td>13</td>
					<td>depth</td>
				</tr>
				<tr>
					<td>Permeability</td>
					<td>Magnetic perpeability mu</td>
					<td>14</td>
					<td>wavelength</td>
				</tr>
				<tr>
					<td>RadDamage</td>
					<td>Radiation damages in shape</td>
					<td>15</td>
					<td>wavelength or depth</td>
				</tr>
			</table>
			<div align="left">
				<p></p>
			</div>
			<div align="center">

			</div>
			<div align="left">
				<p></p>
				<p><b><i>A naming convention is taken in TSplineFit, such that it is always possible
for a fit to retrieve the name of the category and the name of the fit</i></b>.&nbsp;It
is obtained by following the prescriptions described below:</p>
				<ul>
					<li><code><b>the name must have 2 parts, separated by '_':</b></code>
					<ul>
						<li><code><b>Before '_', give a name for the category</b></code>
						<li><code><b>After '_', give a name for the fit</b></code>
					</ul>
					<li><code><b>the title must have 2 parts, separated by &quot; | &quot;:</b></code>
					<ul>
						<li><code><b>Before &quot; | &quot;, give a more detailed explanation for the
category</b></code>
						<li><code><b>After &quot; | &quot;, give a more detailed explanation for the
fit</b></code>
					</ul>
				</ul>
				<p>if you are entering the next elements of the same category i,</p>
				<ul>
					<li><code><b>it is no more necessary to give the part preceeding '_' in the
name, it will be prepended.</b></code>
					<li><code><b>it is no more necessary to give the part preceeding &quot; | &quot;
in the title, it will be prepended.</b></code>
				</ul>
				<p>As an example, suppose your program needs the imaginary part of the index
of refraction of aluminium and of mylar. You could decide to give category number
4 for imaginary part of index of refraction of wrappings and call the 2 <b>TSplineFit</b>
like this:</p>
				<ul>
					<li><code><b>TSplineFit *imalu = new TSplineFit(&quot;IIndexRev_Aluminium&quot;,</b></code>
					<ul>
						<li><code><b>&quot;Im. part of refraction index | Pure aluminium&quot;,4,...);</b></code>
					</ul>
					<li><code><b>TSplineFit *immylar = new TSplineFit(&quot;Mylar&quot;,&quot;mylar
from 3M&quot;,4,...);</b></code>
				</ul>
			</div>
		</div>
		<div align="left">
			<p></p>
			<p></p>
			<div align="center">
				<h3>Families</h3>
				<div align="left">
					<p>The possibility exists for fits within a category to belong to a family of

fits. Grouping fits of the same type, differing only by the value of a parameter,

into a family will offer advantages,</p>
					<div align="left">
						<div align="center">
							<div align="left">
								<i> in particular:</i></div>
						</div>
					</div>
					<ul>
						<div align="left">
							<div align="center">
								<div align="left">
									<li><i>the possibility of interpolation between the fits</i>
									<li><i>the possibility of choosing the element of the family which is closest

to a value p of the parameter of the family to generate random numbers</i>
								</div>
							</div>
						</div>
					</ul>
					<div align="left">
						<div align="center">
							<div align="left">
								<p>Look at methods: <code><b>BelongsToFamily(), FindFirstInFamily(), </b></code><code><b>FindFitInFamily(),

LoadFamily(), </b></code><code><b>V(Int_t,Double_t,Double_t), GetRandom(.,.,.)</b></code></p>
							</div>
						</div>
					</div>
					<p>Let us consider two examples. </p>
					<h4><font color="#ff3333"><b>First example</b>: </font></h4>
					<p>you have at hand 4 measurements of the index of refraction as a function

of wavelength of various glasses, differing only by the amount of lead inside the

glass.</p>
					<ul>
						<li><code>the 1st one with 1% of lead in the glass</code>
						<li><code>the 2nd one with 2% of lead in the glass</code>
						<li><code>the 3rd one with 5% of lead in the glass</code>
						<li><code>the 4th one with 10% of lead in the glass</code>
					</ul>
					<p>It is then natural to group the 4 measurements and fit into a family. For

grouping fits into a family, proceed like this:</p>
					<ul>
						<li>give the same name to all fits of the family (digits will be appended automatically

to the names of all fits of the family in order not to have different fits with the

same name)
						<li>for each fit of the family, call <b><code>BelongsToFamily()</code></b>,

with 1st argument <b>k</b> from <b>0</b> to <b>n-1</b>, where<b> n</b> is the number

of elements in the family, 4 in our example.
						<li>the second argument of <b><code>BelongsToFamily()</code></b> is some parameter

which changes going from one element of the family to the others. In our example,

the percentage of lead.
						<li>the third argument of <b><code>BelongsToFamily()</code></b> is some text

explaining what the second argument is.
					</ul>
					<p><b><code>CINT</code></b> code of this example, including trying to get an

indication of what would be the index of refraction of a glass containing 7% of lead

at a wavelength of 500 nm:</p>
					<ul>
						<li><code><b>Double_t index7_500;</b></code>
						<li><code><b>TSplineFit *ind0, *ind1, *ind2, *ind3;</b></code>
						<li><code><b>ind0 = new TSplineFit(&quot;RefrIndex_leadglass&quot;,&quot;Index

of refraction | glass with 1% lead&quot;,1,...);</b></code>
						<li><code><b>ind0-&gt;BelongsToFamily(0,1.0,&quot;percentage of lead&quot;);</b></code>
						<li><code><b>ind1 = new TSplineFit(&quot;RefrIndex_leadglass&quot;,&quot;Index

of refraction | glass with 2% lead&quot;,1,...);</b></code>
						<li><code><b>ind1-&gt;BelongsToFamily(1,2.0,&quot;percentage of lead&quot;);</b></code>
						<li><code><b>ind2 = new TSplineFit(&quot;RefrIndex_leadglass&quot;,&quot;Index

of refraction | glass with 5% lead&quot;,1,...);</b></code>
						<li><code><b>ind2-&gt;BelongsToFamily(2,5.0,&quot;percentage of lead&quot;);</b></code>
						<li><code><b>ind3 = new TSplineFit(&quot;RefrIndex_leadglass&quot;,&quot;Index

of refraction | glass with 10% lead&quot;,1,...);</b></code>
						<li><code><b>ind3-&gt;BelongsToFamily(3,10.0,&quot;percentage of lead&quot;);</b></code>
						<li><code><b>index7_500 = ind0-&gt;V(4,500.0,7.0);</b></code>
					</ul>
					<p>After these calls to BelongsToFamily are done, the names of the 4 fits will

be:</p>
					<ul>
						<li><code><b>&quot;RefrIndex_leadglass_000&quot;, &quot;RefrIndex_leadglass_001&quot;,

&quot;RefrIndex_leadglass_002&quot;,</b></code><code><b>&quot;RefrIndex_leadglass_003&quot;.</b></code>
					</ul>
					<p>Same digits will also have been appended to the titles.</p>
					<h4><font color="#ff3333">Second example:</font></h4>
					<p>You have a family of fits giving the distribution for the energy deposited

in 1 cm of CsI by the crossing of muons.&nbsp;The parameter differing inside the

family is the energy of the muon.&nbsp;You want to use this family to get a random

number distributed as the energy deposited for a muon of 25 Gev.&nbsp;Calling the

version of GetRandom() reserved for families of fits, the fit inside the family which

has a parameter closest to 25 Gev will be chosen to generate the random number.</p>
					<p></p>
					<h4><font color="#ff3333">Usage:</font></h4>
					<p><b>Always define family members in <font color="#b22222">increasing</font>
values of the parameter!</b> The first element of the family has the smallest value
of the parameter and the last element of the family has the largest value of the
parameter! If you have to use, from your application, a family of fits rather than
simply a fit, proceed like this:</p>
					<ul>
						<li>keep inside your code the pointer <code>p0</code> to the first element
of the family and call:
						<li><code>N=p0-&gt;LoadFamily()</code>
						<li>keep <code>N</code>: the number of elements in the family.
					</ul>
					<p>Then you will be able to call  V(N,x,p) or GetRandom(N,p).</p>
					<p></p>
				</div>
			</div>
		</div>
		<div align="center">
			<div align="center">
				<h3>Fit collection and database</h3>
			</div>
			<div align="left">
				<div align="center">
					<div align="left">
						<p>All fits defined in an application using <b>SplineFit</b> are held into

an ordered collection (static variable <code><b>TSplineFit::fgFits</b></code>) and

may be retrieved at will. <b><code>TSplineFit</code></b> allows also, with the method

<b><code>UpdateFile()</code></b>, to build a kind of database in the form of a <b><code>Root</code></b>

file, where the fit is searched for (static method <b><code>TSplineFit::FindFit()</code></b>)

in case it is not found inside the collection <code><b>TSplineFit::fgFits</b></code>.</p>
					</div>
				</div>
				<p>Each time you define a fit, without intervention of the user, this fit is


put into the static collection <code><b>TSplineFit::fgFits</b></code>, so that it


can be retrieved later on in the program by the method <b><code>FindFit()</code></b>,


giving name and (optionnaly) category of the fit. The retrieval is faster if you


give the category. If the program tries to access a fit which is not in the collection

<code><b>TSplineFit::fgFits</b></code>, i.e. has not been defined in the program,


a <b><code>Root</code></b> &quot;database&quot; file is searched and if found in

the database, the fit is inserted into the collection. This allows to define fits

once for all, which are at hand for many different programs. We improperly call it

a database file, although it is simply a Root file, because we have implemented in

<b><code>TSplineFit</code></b> methods to retrieve specific elements [<b><code>FindFit()</code></b>],

to delete specific elements [<b><code>RemoveFitFromFile()</code></b>] and methods

to order elements in the file [<b><code>OrderFile()</code></b>].&nbsp;We have given

the extension <b><code>.rdb</code></b> and not <b><code>.root</code></b> to this

file to stress this point, but user can change this at will.</p>
				<p>The default name (==treename) of the &quot;database&quot; file is</p>
				<ul>
					<li><code><b>TString *TSplineFit::fgFileName = &quot;SplineFitDB.rdb&quot;;</b></code>
				</ul>
				<p>So when unpacking code and source of SplineFit, look at the file <code>SplineFitDB.rdb</code>
in the directory &quot;<code>database</code>&quot; and place it into the same directory
in which you intend to launch the application using SplineFit :</p>
				<ul>
					<li>in the same directory as SplineFit.exe if you intend to launch SplineFit.exe,
					<li>or in the same directory as Litrani, if you intend to launch the version
of Litrani using Splinefit,					<li>or in the same directory as your application,
if your application is using SplineFit.
				</ul>
				<p>By that you will inherit of all fits allowing the examples of use of Litrani
to work. If this position does not suit your needs, you have simply to redefine the
static pointer <b><code>TSplineFit::fgFileName</code></b>.&nbsp;It
is not necessary to recompile the program: this static variable can be accessed from


the <code><b>CINT</b></code> code, so that you can redefine it in the first line


of your <code><b>CINT</b></code> code, for example like this:</p>
				<ul>
					<li><code><b>if (TSplineFit::fgFileName) delete TSplineFit::fgFileName;</b></code>
					<li><code><b>TSplineFit::fgFileName = new TString(&quot;c:\\mydatabases\\SplineFitDB.rdb&quot;);</b></code>
				</ul>
				<p>If you define a fit for the first time and want it to be stored into the database,
use the method <b><code>UpdateFile(Bool_t=kFALSE)</code></b>. Be careful that the
argument must be <b><code>kFALSE, </code></b>or absent! Use a <b><code>kTRUE</code></b>
argument only in the case the database file does not exist and you are creating it
by entering its first fit! If this fit (or a fit with the same name) is already in
the database file, it will not be inserted: a check is done with the method <code><b>VerifyNotInFile().
</b></code>Remember that by convention, a fit of category 0 is <b><font color="#b22222">not</font></b>
put inside the database file. The static method <code><b>TSplineFit::ShowFitsInFile()</b></code>
prints the list of all fits available in the file. The static method <b><code>TSplineFit::DrawFitsInFile()</code></b>
draws each fit in the file, in turn.</p>
				<p>If you are short of memory and your program needs to handle a lot of fits,


you can call the method <b><code>ReduceMemory()</code></b>.&nbsp;Calling it, you


preserve only what is necessary in order to obtain y(x) with the method <b><code>V(x)</code></b>.


All measurements, matrices of the problem, are lost. Do never call <b><code>ReduceMemory()</code></b>


before storing a fit into the &quot;database&quot; file.</p>
				<p>The collection  being a <b><code><a href="http://root.cern.ch/root/html/TObjArray.html">TObjArray</a></code></b>,


the fit number i can be accessed very simply by <b><code>fit = TSplineFit::fgFits[i]</code></b>.


Notice that the fits are ordered in the collection <code><b>TSplineFit::fgFits</b></code>.&nbsp;They

are ordered in increasing category number, and they are ordered in alphabetical order

within a category. When you put new fits into the database file, these new fits are

appended at the end, so that the database file is no more ordered.&nbsp;If you want

it to be ordered, call the static method <b><code>TSplineFit::OrderFile()</code></b>.


If you want to remove a fit from the database file, call the static method <b><code>TSplineFit::RemoveFitFromFile()</code></b>.</p>
				<p></p>
			</div>
			<div align="center">
				<h3>Random number generation</h3>
			</div>
			<div align="left">
				<p>If the user wants to generate random numbers according to the fitted distribution,


the method <b><code>UseForRandom()</code></b> has to be called for allowing it. Notice


that the condition is that the fitted distribution is never negative, so that the


distribution must have been created with a positive lower bound. Then, the method


<b><code>GetRandom()</code></b> get a random number according to the fitted distribution.</p>
				<p>The method used is to create an histogram <b><code>fHGenRandom</code></b>


whose bin content is according the fitted distribution and to use the Root method


<b><code>TH1::GetRandom()</code></b>. The number of channels of this histogram is


given by the static variable <code><b>TSplineFit::fgNChanRand</b></code>, which has


a default value of 128 and can be changed by the user before calling <b><code>UseForRandom()</code></b>.


To verify that the random numbers are indeed generated according to the fit distribution,


call the method <b><code>ShowRandom()</code></b>. To get a random number according


to the fitted distribution, call the method <b><code>TSplineFit::GetRandom()</code></b>.</p>
				<p>The possibility of generating random numbers according to a fitted distribution


is quite useful.&nbsp;Think for instance to the case where you have entered <b><code>dE/dx</code></b>


distributions for the energy loss per cm of muons of a given energy in some material.&nbsp;You


need then to generate random numbers according to this distribution to simulate energy


deposit of muons in your Monte-Carlo.</p>
				<p></p>
			</div>
			<div align="center">
				<h3>2D or 3D fits</h3>
			</div>
			<div align="left">
				<p>A &quot;trick&quot; is used by <b>SplineFit</b> to extend the possibilities

of <b>TSplineFit</b> to <b>2D</b> or <b>3D</b>.&nbsp;This trick is explained in the

class description of <b><a href="TZigZag.html">TZigZag</a></b>. Be aware that it

is a trick and we never use 2D or 3D splines, i.e. splines depending upon (x,y) or

(x,y,z)! But the trick works and a nice example [<b>Example2D.C</b>] of it is privided

among the examples of use of <b>SplineFit</b>. In case of 2D or 3D fits, the data

to be fitted must be provided in histograms of type <b><a href="http://root.cern.ch/root/html/TH2D.html">TH2D</a></b>

or <b><a href="http://root.cern.ch/root/html/TH3D.html">TH3D</a></b>. Use the 7th

or 8th constructor.</p>
				<p></p>
			</div>
			<div align="center">
				<h3>Example of use</h3>
			</div>
			<div align="left">
				<p><b>SplineFit</b> is downloaded from the web with its sources.&nbsp;Examples

of use are provided in the directory <b><code>FitMacros</code></b>. In fact, FitMacros
contains the definitions of the fits used in the examples of Litrani.</p>
				<p></p>
				<div align="center">
					<div align="center">
						<h3>Visualization and quality of the fit</h3>
					</div>
					<div align="left">
						<p>In case of 1D, the method <b><code>DrawFit()</code></b> draws superimposed

graph of the measured points and of the fit.</p>
						<p>In case of 2D or 3D, call the method <b><code>DrawData()</code></b> to see

the data, and then <b><code>DrawFit()</code></b> to see the fit.</p>
						<p>All drawing are done by the class <b><code>TOnePadDisplay</code></b>. A

global pointer, <b><code>gOneDisplay</code></b> points towards <b><code>TOnePadDisplay</code></b>.

This global pointer is not only global, it is also available from your <b><code>CINT</code></b>

code. There are 3 ways of using it:</p>
						<ol>
							<li>Simply don't care.&nbsp;In that case, <b><code>gOneDisplay</code></b>

will be booked automatically at the first call to a <b><code>Draw..</code></b>. method.&nbsp;In

that case, you will have all the defaults of <b><code>TOnePadDisplay</code></b>.

The 3 labels displayed in addition to the fit will be general labels, not specific

to the fit displayed.
							<li>Just call <b><code>SetDefaultLabels()</code></b>, without arguments, from

your <b><code>CINT</code></b> code, after the full definition of the fit and before

calling any of the <b><code>Draw..</code></b>.  methods.&nbsp;<b><code>gOneDisplay</code></b>

will be booked by this call, you will have all the defaults of <b><code>TOnePadDisplay</code></b>,

but at least the 3 labels drawn will be specific to the fit
							<li>Book <b><code>gOneDisplay</code></b> yourself from your <b><code>CINT</code></b>

code, by a call to the constructor of <b><code>TOnePadDisplay</code></b>.&nbsp;Then

change all the class variables of <b><code>TOnePadDisplay</code></b> that you want

to be changed.&nbsp;All class variable of <b><code>TOnePadDisplay</code></b> are

public.&nbsp;And finally call <b><code>gOneDisplay-&gt;BookCanvas()</code></b>. So

<b><code>TOnePadDisplay</code></b> will suit your needs and taste.
						</ol>
						<p> </p>
						<p></p>
					</div>
				</div>
			</div>
			<div align="center">
				<h3>Arguments of the constructor of TSplineFit</h3>
				<h3></h3>
			</div>
			<table border="2" cellpadding="4" cellspacing="2" bgcolor="#faebd7">
				<caption>
					<h3>For the first constructor</h3>
				</caption>
				<tr>
					<td><b><code><font size="+1">name of argument</font></code></b></td>
					<td>
						<div align="center">
							<b><code><font size="+1">default</font></code></b></div>
					</td>
					<td>
						<div align="center">
							<b><code><font size="+1">meaning</font></code></b></div>
					</td>
				</tr>
				<tr>
					<td><code><b>name</b></code></td>
					<td><b><code>no default</code></b></td>
					<td><code><b>name of this spline fit</b></code></td>
				</tr>
				<tr>
					<td><code><b>title</b></code></td>
					<td><b><code>no default</code></b></td>
					<td><code><b>title of this spline fit</b></code></td>
				</tr>
				<tr>
					<td><code><b>cat</b></code></td>
					<td><b><code>no default</code></b></td>
					<td><code><b>category to which this spline fit belongs (arbitrary)</b></code></td>
				</tr>
				<tr>
					<td><code><b>M</b></code></td>
					<td><b><code>no default</code></b></td>
					<td><code><b>number of measurements</b></code></td>
				</tr>
				<tr>
					<td><code><b>m</b></code></td>
					<td><b><code>no default</code></b></td>
					<td><code><b>number of measurements per spline.&nbsp;If m==1, interpolation,


not fit</b></code></td>
				</tr>
				<tr>
					<td><code><b>x</b></code></td>
					<td><b><code>no default</code></b></td>
					<td><code><b>list of abscissa values of measurements in increasing order</b></code></td>
				</tr>
				<tr>
					<td><b><code>y</code></b></td>
					<td><b><code>no default</code></b></td>
					<td><code><b>list of values of y[i] measurements at x[i]</b></code></td>
				</tr>
				<tr>
					<td><code><b>sig</b></code></td>
					<td><code><b>default 0.0</b></code></td>
					<td>
						<dl>
							<dt><code><b>errors on y[i] measurements at x[i].</b></code>
							<dt><code><b>If sig==0, then all errors assumed to be 1.0</b></code>
						</dl>
					</td>
				</tr>
				<tr>
					<td><code><b>lowbounded</b></code></td>
					<td><code><b>default false</b></code></td>
					<td><code><b>y values have a lower bound</b></code></td>
				</tr>
				<tr>
					<td><code><b>ylow</b></code></td>
					<td><code><b>default 0.0</b></code></td>
					<td><code><b>lower bound for y. [irrelevant if lowbounded false]</b></code></td>
				</tr>
				<tr>
					<td><code><b>upbounded</b></code></td>
					<td><code><b>default false</b></code></td>
					<td><b><code>value have an upper bound. The values returned by V() are cut at
yup</code></b></td>
				</tr>
				<tr>
					<td><code><b>yup</b></code></td>
					<td><code><b>default 1.0</b></code></td>
					<td><code><b>upper bound for y. [irrelevant if upbounded false]</b></code></td>
				</tr>
				<tr>
					<td><code><b>xmin</b></code></td>
					<td><code><b>default 1.0</b></code></td>
					<td><code><b>lower bound in x for the display of the fit</b></code></td>
				</tr>
				<tr>
					<td><code><b>xmax</b></code></td>
					<td><code><b>default -1.0</b></code></td>
					<td>
						<dl>
							<dt><code><b>upper bound in x for the display of the fit</b></code>
							<dt><code><b>The default for xmin and xmax are set in such a way that the</b></code>
							<dt><code><b>constructor will recalculate reasonable values</b></code>
						</dl>
					</td>
				</tr>
				<tr>
					<td><code><b>debug</b></code></td>
					<td><code><b>default false</b></code></td>
					<td><code><b>if true, print matrix and vector of the problem</b></code></td>
				</tr>
			</table>
			<div align="left">
				<p></p>
				<p></p>
			</div>
			<div align="center">
				<h3>Minimum and maximum of the distribution</h3>
			</div>
			<div align="left">
				<p>Method <b><code>MinMax()</code></b> finds the lowest among the minima of the


fitted distribution, if there is at least one minimum, and the biggest among the


maxima of the distribution, if there is at least one maximum.</p>
				<ul>
					<li>minimum in mathematical sense : point where y' = 0 and y&quot; &lt; 0

					<li>maximum in mathematical sense : point where y' = 0 and y&quot; &gt; 0

				</ul>
				<p>Important restriction: this method cannot be applied when you have asked for
a lower bound for the fit, and also not in the case of an interpolation instead of
a fit. Do not call it in this case!</p>
				<p></p>
			</div>
			<div align="center">
				<h3>Data to be fitted taken from a graph or from an histogram</h3>
			</div>
			<div align="left">
				<p>Using the 4th constructor of <b><code>TSplineFit</code></b>, you can take


the data to be fitted from a graph of type <code><b><a href="http://root.cern.ch/root/html/TGraphErrors.html">TGraphErrors</a></b></code>.&nbsp;Using

the 5th or the 6th constructor of <b><code>TSplineFit</code></b>, you can take the


data to be fitted from an histogram of type <b><code><a href="http://root.cern.ch/root/html/TH1D.html">TH1D</a></code></b>.


When the fit of class <b><code>TSplineFit</code></b> has taken its data from an histogram,


it opens new possibilities: <b><code>TSplineFit</code></b> creates in that case a


<b><code><a href="http://root.cern.ch/root/html/TF1.html">TF1</a></code></b> object,


calling the function <b><code>SplineFitFunc()</code></b> which is the fit function.&nbsp;This


<b><code><a href="http://root.cern.ch/root/html/TF1.html">TF1</a></code></b> object


is then stored into the collection of associated functions of the histogram, so that


when the histogram is plotted, the fit is also plotted.</p>
				<p>An other benefit of <b><code>TSplineFit</code></b> applied to the content


of an histogram is to offer a solution to the following problem concerning the errors


in an histogram. The distribution of hits inside an histogram containing <b>N</b>


hits in all is described by the multinomial distribution, <font face="Symbol"><b>e</b></font><b><sub>i</sub></b>


being the unknown probability for a hit to fall into bin i. The sum of all <font face="Symbol"><b>e</b></font><b><sub>i</sub></b>


of all bins is 1. The <font face="Symbol"><b>e</b></font><b><sub>i</sub></b> being


unknown and the number of hits inside bin i being <b>n<sub>i</sub></b>, the multinomial


distribution gives <font face="Symbol"><b>e</b></font><b><sub>i</sub></b> = <b>n<sub>i</sub></b>


/ <b>N</b> as the most probable value for <font face="Symbol"><b>e</b></font><b><sub>i</sub></b>.


The error on bin i is then <b><code>Sqrt(N*</code></b><font face="Symbol"><b>e</b></font><b><sub>i</sub><code>*(1-</code></b><font face="Symbol"><b>e</b></font><b><sub>i</sub><code>))


= Sqrt(</code>n<sub>i</sub><code>*(1-</code>n<sub>i</sub><code>/N))</code></b>. The


horrible feature of this formula is that it gives an error of 0 (i.e. a <b><i>infinite


weight</i></b>!) for the bins containing 0 hits. It means that in a mathematically


strict point of view, it is impossible to make a fit on an histogram having some


bins with 0 content. The weight of these bins being infinite, the other bins are


negligible and the only mathematically correct solution for a fit on the histogram


is <b><code>y==0</code></b> for all <b>x</b>! The way physicists generally turn around
this problem is to <i><font color="#b22222"><b>arbitrary</b></font></i> set the weight
of these bins with 0 content to 0, <i><b>going from an extremely wrong solution to
an other extremely wrong solution! </b></i>Indeed

the fact of having received 0 hit in some bin is a clear indication that the <font face="Symbol"><b>e</b></font><b><sub>i</sub></b>


of this bin is small, and that the weight of this bin, however not being infinite,


is surely big. Putting it to 0 is totally wrong.</p>
				<p><b><code>TSplineFit</code></b> offers a way out of this awful dilemma:</p>
				<ol>
					<li>call the 5th or the 6th constructor of <b><code>TSplineFit</code></b> giving


the histogram for which you want a reasonable (not 0, not infinite) estimation of


the errors.

					<li>do not forget to ask for a lower bound of 0 (3rd argument of the 4th constructor)

					<li>use the result of the spline fit to get an estimation of the <font face="Symbol"><b>e</b></font><b><sub>i</sub></b>


for each bin. You can be absolutly sure to never obtain a value of <font face="Symbol"><b>e</b></font><b><sub>i</sub>


= 0</b>, due to the way low bounded fits are implemented in <b><code>TSplineFit</code></b>.

					<li>recalculate the errors of the fitted bins using the obtained <font face="Symbol"><b>e</b></font><b><sub>i</sub></b>


and the formula <b>Sqrt(N*</b><font face="Symbol"><b>e</b></font><b><sub>i</sub>*(1-</b><font face="Symbol"><b>e</b></font><b><sub>i</sub>))</b>.


Put these new errors in the histogram using the method <b><code>TH1::SetBinError()</code></b>.

					<li>then you are able to do fits on the histogram which are not meaningless!!

				</ol>
				<p>It is exactly what the method <b><code>TSplineFit::ErrorsFromFit()</code></b>


does! Look at this method.</p>
				<p>Careful readers have certainly detected here a snake biting its tail: we want
reasonable errors to do a fit and propose the result of the fit to have reasonable
errors. How to do the first fit without having yet reasonable errors? For the first
fit, we make a check of the errors. If we detect bins with 0 error, we replace all
the errors of all bins by errors calculated using the formula obtained <b>when taking
the multinomial distribution not as a likelihood function, but as a weight function</b>
[for more details, contact me]. These errors are never 0, even if the content of
the bin is 0. This is done by the method <b>MultinomialAsWeight()</b>. Then we proceed


by iteration. The user is free to repeat the fit as many times as he wants, having


then better and better errors, and smaller lower limits for the errors. The <b><code>CINT</code></b>


code to do this, making for instance the fit 3 times, is (<b>h</b> being a pointer


on a <b><code><a href="http://root.cern.ch/root/html/TH1D.html">TH1D</a></code></b>


histogram, and <b>eps</b> an absolute small lower bound on the errors, in order to


prevent problems in fitting):</p>
				<p></p>
				<ul>
					<li><b><code>const Double_t eps = 1.0e-2;</code></b>
					<li><b><code>TSplineFit *sf = new TSplineFit(h,0,3,kTRUE);</code></b>
					<li><b><code>sf-&gt;ErrorsFromFit();</code></b>
					<li><b><code>sf-&gt;GetDataFromHist(eps);</code></b>
					<li><b><code>sf-&gt;RedoFit();</code></b>
					<li><b><code>sf-&gt;ErrorsFromFit();</code></b>
					<li><b><code>sf-&gt;GetDataFromHist(eps);</code></b>
					<li><b><code>sf-&gt;RedoFit();</code></b>
				</ul>
			</div>
			<div align="left">
				<p></p>
				<p>Let us now describe the mathematics behind fits of spline.</p>
				<p></p>
			</div>
			<h2>The spline fit problem</h2>
		</div>
		<div align="left">
			<p></p>
			<p>We expose here the problem of fitting, not interpolating, data points using


a collection of splines. We define splines as being polynomials of 3rd degree, with


the characteristics that at the junction <code><b><font face="Symbol">z</font><sub>k


</sub></b></code>between 2 splines:</p>
			<ol>
				<li>the ordinate value <code><b>y<sub>L</sub>(<font face="Symbol">z</font><sub>k</sub>)</b></code>


of the left spline is equal to the ordinate value <code><b>y<sub>R</sub>(<font face="Symbol">z</font><sub>k</sub>)</b></code>


of the right spline.
				<li>the 1st derivative <code><b>y'<sub>L</sub>(<font face="Symbol">z</font><sub>k</sub>)</b></code>of


the left spline is equal to the 1st derivative <code><b>y'<sub>R</sub>(<font face="Symbol">z</font><sub>k</sub>)


</b></code>of the right spline.
				<li>the 2nd derivative <code><b>y&quot;<sub>L</sub>(<font face="Symbol">z</font><sub>k</sub>)</b></code>of


the left spline is equal to the 2nd derivative <code><b>y&quot;<sub>R</sub>(<font face="Symbol">z</font><sub>k</sub>)


</b></code>of the right spline.
				<li>Beyond that, the collection of splines must fit as good as possible the list


of data points.
			</ol>
			<p>Let us formulate the problem mathematically. We have:</p>
			<ul>
				<li>the <b>N</b> intervals <b><code>[</code></b><code><b><font face="Symbol">z</font></b></code><b><code><sub>k-1</sub>,


</code></b><code><b><font face="Symbol">z</font></b></code><b><code><sub>k</sub>]


</code></b>, the k<sup>th</sup> interval being defined as contained between <b><code>[</code></b><code><b><font face="Symbol">z</font></b></code><b><code><sub>k-1</sub>,


</code></b><code><b><font face="Symbol">z</font></b></code><b><code><sub>k</sub>]</code></b>,


each interval containing <b>m</b> data points and one spline. There are <b><code>N+1</code></b>


values <code><b><font face="Symbol">z</font></b></code><b><code><sub>k</sub> 0&lt;=k&lt;=N</code></b>.


The left of the first interval is <code><b><font face="Symbol">z</font></b></code><b><code><sub>0</sub></code></b>,


the right of the Nth interval is <code><b><font face="Symbol">z</font></b></code><b><code><sub>N</sub></code></b>.
				<li>the <b>M</b> data points <code><b>(x<sub>j</sub>, y<sub>j</sub>, <font face="Symbol">s</font><sub>j</sub>)


j=0..M-1</b></code>. But we prefer to note the data points <code><b>(x<sub>ki</sub>,


y<sub>ki</sub>, <font face="Symbol">s</font><sub>ki</sub>) k=1..N, i=0..m-1</b></code>.


The indices <code><b><sub>ki</sub></b></code> meaning the <code><b>i<sup>th</sup></b></code>


point belonging to interval <code><b>k</b></code>.There are <b>m</b> points per interval,


except perhaps in the last interval, in case <b>M</b> is not divisible by N. <code><b><font face="Symbol">s</font><sub>ki</sub></b></code>


is the error on <code><b>y<sub>ki</sub></b></code>.
				<li>The intermediate values <code><b><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b>


k=1..N-1</b></code> are defined by: <code><b><font face="Symbol">z</font><sub>k</sub>


= (x<sub>k,m-1</sub> + x<sub>k+1,0</sub>)/2</b></code>.&nbsp;It means that the end


point of interval <b>k</b> is exactly in the middle of the last point of interval<b>


k</b> and the first point of interval <b>k+1</b>.
				<li>The extremal values <code><b><font face="Symbol">z</font></b></code><b><code><sub>0</sub></code></b>,


<code><b><font face="Symbol">z</font></b></code><b><code><sub>N</sub></code></b>


are defined by:
				<ul>
					<li><code><b><font face="Symbol">z</font><sub>0</sub> = x<sub>10</sub> - (x<sub>11</sub>-x<sub>10</sub>)


= (3x<sub>10</sub> - x<sub>11</sub>)/2</b></code>
					<li><code><b><font face="Symbol">z</font><sub>N</sub> = x<sub>N,m-1</sub> +


(x<sub>N,m-1</sub>-x<sub>N,m-2</sub>)/2 = (3x<sub>N,m-1</sub> - x<sub>N,m-2</sub>)/2</b></code>
				</ul>
				<li>We write the spline of interval k as : <code><b>y = a<sub>k</sub> + b<sub>k</sub>x


+ c<sub>k</sub>x<sup>2</sup> + d<sub>k</sub>x<sup>3</sup></b></code>.
				<li>Condition (1) about equality of ordinate values at points <code><b><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b>


is now:
				<ul>
					<li><code><b>a<sub>k+1</sub> + b<sub>k+1</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b>


+ c<sub>k+1</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b><sup>2</sup>


+ d<sub>k+1</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b><sup>3


</sup>=<sup> </sup>a<sub>k</sub> + b<sub>k</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b>


+ c<sub>k</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b><sup>2</sup>


+ d<sub>k</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b><sup>3</sup></b></code>.
				</ul>
				<li>Condition (2) about equality of the derivatives at points <code><b><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b>


is now:
				<ul>
					<li><code><b>b<sub>k+1</sub></b></code><code><b> + 2c<sub>k+1</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b>


+ 3d<sub>k+1</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b><sup>2


</sup>=<sup> </sup>b<sub>k</sub></b></code><code><b> + 2c<sub>k</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b>


+ 3d<sub>k</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b><sup>2</sup></b></code>
				</ul>
				<li>Condition (3) about equality of the second derivatives at points <code><b><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b>


is now:
				<ul>
					<li><code><b>c<sub>k+1</sub></b></code><code><b> + 3d<sub>k+1</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b><code><b><sup>


</sup>=<sup> </sup></b></code><code><b>c<sub>k</sub></b></code><code><b> + 3d<sub>k</sub><font face="Symbol">z</font></b></code><b><code><sub>k</sub></code></b>.
				</ul>
			</ul>
			<p>With these definitions, we can formulate the likelihood function of the problem


in the following way:</p>
		</div>
		<div align="center">
			<table border="1" cellpadding="2" cellspacing="1" bgcolor="white">
				<tr>
					<td><font face="Symbol" size="+1"><code><b>L</b></code></font></td>
					<td><code><b><font size="+1">=</font></b></code></td>
					<td><code><b><font face="Symbol" size="+1">S</font><font size="+1"><sub>k=1</sub><sup>N


</sup>[ </font><font face="Symbol" size="+1">S</font><font size="+1"><sub>i=0</sub><sup>m-1</sup>


(y<sub>ki</sub> - a<sub>k</sub> - b<sub>k</sub>x<sub>ki</sub> - c<sub>k</sub>x<sub>ki</sub><sup>2</sup>


- d<sub>k</sub>x<sub>ki</sub><sup>3</sup>)<sup>2</sup>/</font><font face="Symbol" size="+1">s</font><font size="+1"><sub>ki</sub><sup>2</sup>]</font></b></code></td>
				</tr>
				<tr>
					<td></td>
					<td><code><b><font size="+1">+</font></b></code></td>
					<td><code><b><font face="Symbol" size="+1">S</font><font size="+1"><sub>k=1</sub><sup>N-1


</sup>[ </font><font face="Symbol" size="+1">l</font><font size="+1"><sub>ka</sub>


( a<sub>k+1</sub> + b<sub>k+1</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub>


+ c<sub>k+1</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub><sup>2</sup>


+ d<sub>k+1</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub><sup>3


</sup>-<sup> </sup>a<sub>k</sub> - b<sub>k</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub>


- c<sub>k</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub><sup>2</sup>


- d<sub>k</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub><sup>3</sup>


)]</font></b></code></td>
				</tr>
				<tr>
					<td></td>
					<td><code><b><font size="+1">+</font></b></code></td>
					<td><code><b><font face="Symbol" size="+1">S</font><font size="+1"><sub>k=1</sub><sup>N-1</sup>[


</font><sup></sup><font face="Symbol" size="+1">l</font><font size="+1"><sub>kb</sub>


( b<sub>k+1</sub> + 2c<sub>k+1</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub>


+ 3d<sub>k+1</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub><sup>2


</sup>-<sup> </sup>b<sub>k</sub> - 2c<sub>k</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub>


- 3d<sub>k</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub><sup>2


</sup>)]</font></b></code></td>
				</tr>
				<tr>
					<td></td>
					<td><code><b><font size="+1">+</font></b></code></td>
					<td><code><b><font face="Symbol" size="+1">S</font><font size="+1"><sub>k=1</sub><sup>N-1</sup>[


</font><sup></sup><font face="Symbol" size="+1">l</font><font size="+1"><sub>kc</sub>


( c<sub>k+1</sub> + 3d<sub>k+1</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub><sup>


</sup>-<sup> </sup>c<sub>k</sub> - 3d<sub>k</sub></font><font face="Symbol" size="+1">z</font><font size="+1"><sub>k</sub>


)]</font></b></code></td>
				</tr>
			</table>
		</div>
		<div align="left">
			<p>The first line insures that the splines go at best through the data points,


the 3 following lines insures, through the Lagrange parameters <code><b><font face="Symbol">l</font><sub>ka</sub>,


<font face="Symbol">l</font><sub>kb</sub>, <font face="Symbol">l</font><sub>kc</sub></b></code>,


that the 3 conditions (1), (2), (3) are satisfied. Let us derive by the 4 unknown


<b><code>a<sub>k</sub>, b<sub>k</sub>, c<sub>k</sub>, d<sub>k </sub></code></b>and


by the 3 Lagrange parameters <code><b><font face="Symbol">l</font><sub>ka</sub>,


<font face="Symbol">l</font><sub>kb</sub>, <font face="Symbol">l</font><sub>kc</sub></b></code>.


Let us define:</p>
		</div>
		<div align="center">
			<table border="1" cellpadding="2" cellspacing="1" bgcolor="white">
				<tr>
					<td>
						<p><b><code><font face="Symbol" size="+1">k</font><font size="+1"><sub>1k</sub>


= 1 - </font><font face="Symbol" size="+1">d</font><font size="+1"><sub>1k</sub></font></code></b></p>
					</td>
					<td width="200"></td>
					<td><b><code><font face="Symbol" size="+1">k</font><font size="+1"><sub>Nk</sub>


= 1 - </font><font face="Symbol" size="+1">d</font><font size="+1"><sub>Nk</sub></font></code></b></td>
				</tr>
			</table>
		</div>
		<div align="left">
			<div align="left">
				<p>Then we can write these derivatives:</p>
			</div>
		</div>
		<div align="center">
			<table border="1" cellpadding="2" cellspacing="1" bgcolor="white">
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><b><code><font face="Symbol" size="+1">d</font><font size="+1">L/</font><font face="Symbol" size="+1">dl</font><font size="+1"><sub>ka</sub></font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><code><font size="+1"><b>a<sub>k+1</sub> + b<sub>k+1</sub><font face="Symbol">z</font><sub>k</sub>


+ c<sub>k+1</sub><font face="Symbol">z</font><sub>k</sub><sup>2</sup> + d<sub>k+1</sub><font face="Symbol">z</font><sub>k</sub><sup>3


</sup>-<sup> </sup>a<sub>k</sub> - b<sub>k</sub><font face="Symbol">z</font><sub>k</sub>


- c<sub>k</sub><font face="Symbol">z</font><sub>k</sub><sup>2</sup> - d<sub>k</sub><font face="Symbol">z</font><sub>k</sub><sup>3</sup></b></font></code></td>
					<td rowspan="3"><b><code><font size="+1">k</font></code></b>
						<p><b><code><font size="+1">=</font></code></b></p>
						<p><b><code><font size="+1">1..</font></code></b></p>
						<p><b><code><font size="+1">N-1</font></code></b></p>
					</td>
				</tr>
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><b><code><font face="Symbol" size="+1">d</font><font size="+1">L/</font><font face="Symbol" size="+1">dl</font><font size="+1"><sub>kb</sub></font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><code><font size="+1"><b>b<sub>k+1</sub> + 2c<sub>k+1</sub><font face="Symbol">z</font><sub>k</sub>


+ 3d<sub>k+1</sub><font face="Symbol">z</font><sub>k</sub><sup>2 </sup>-<sup> </sup>b<sub>k</sub>


- 2c<sub>k</sub><font face="Symbol">z</font><sub>k</sub> - 3d<sub>k</sub><font face="Symbol">z</font><sub>k</sub><sup>2</sup></b></font></code></td>
				</tr>
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><b><code><font face="Symbol" size="+1">d</font><font size="+1">L/</font><font face="Symbol" size="+1">dl</font><font size="+1"><sub>kc</sub></font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><code><font size="+1"><b>c<sub>k+1</sub> + 3d<sub>k+1</sub><font face="Symbol">z</font><sub>k</sub><sup>


</sup>-<sup> </sup>c<sub>k</sub> - 3d<sub>k</sub><font face="Symbol">z</font><sub>k</sub></b></font></code></td>
				</tr>
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><b><code><font face="Symbol" size="+1">d</font><font size="+1">L/</font><font face="Symbol" size="+1">d</font><font size="+1">a<sub>k</sub></font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><font size="+1"><code><b><font face="Symbol">S</font></b><b><sub>i=0</sub></b><b><sup>m-1</sup></b><b>


(-2/</b><b><font face="Symbol">s</font></b><b><sub>ki</sub></b><b><sup>2</sup></b><b>)(y</b><b><sub>ki</sub></b><b>-a</b><b><sub>k</sub></b><b>-b</b><b><sub>k</sub></b><b>x</b><b><sub>ki</sub></b><b>-c</b><b><sub>k</sub></b><b>x</b><b><sub>ki</sub></b><b><sup>2</sup></b><b>-d</b><b><sub>k</sub></b><b>x</b><b><sub>ki</sub></b><b><sup>3</sup></b><b>)


+ </b></code><b><code><font face="Symbol">k</font></code><code><sub>1k </sub></code></b><code><b><font face="Symbol">l</font></b><b><sub>k-1,a


- </sub></b></code><b><code><font face="Symbol">k</font></code><code><sub>Nk </sub></code></b><code><b><font face="Symbol">l</font></b><b><sub>k,a</sub></b><b>


</b></code></font></td>
					<td rowspan="7"><b><code><font size="+1">k</font></code></b>
						<p><b><code><font size="+1">=</font></code></b></p>
						<p><b><code><font size="+1">1..</font></code></b></p>
						<p><b><code><font size="+1">N</font></code></b></p>
					</td>
				</tr>
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><b><code><font face="Symbol" size="+1">d</font><font size="+1">L/</font><font face="Symbol" size="+1">d</font><font size="+1">b<sub>k</sub></font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><font size="+1"><code><b><font face="Symbol">S</font></b><b><sub>i=0</sub></b><b><sup>m-1</sup></b><b>


(-2x</b><b><sub>ki</sub></b><b>/</b><b><font face="Symbol">s</font></b><b><sub>ki</sub></b><b><sup>2</sup></b><b>)(y</b><b><sub>ki</sub></b><b>-a</b><b><sub>k</sub></b><b>-b</b><b><sub>k</sub></b><b>x</b><b><sub>ki</sub></b><b>-c</b><b><sub>k</sub></b><b>x</b><b><sub>ki</sub></b><b><sup>2</sup></b><b>-d</b><b><sub>k</sub></b><b>x</b><b><sub>ki</sub></b><b><sup>3</sup></b><b>)


+ </b></code><b><code><font face="Symbol">k</font></code><code><sub>1k </sub></code></b><code><b>(</b><b><font face="Symbol">l</font></b><b><sub>k-1,a


</sub></b><b><font face="Symbol">z</font></b><b><sub>k-1</sub></b><b> + </b><b><font face="Symbol">l</font></b><b><sub>k-1,b</sub></b><b>)</b><b><sub>


- </sub></b></code><b><code><font face="Symbol">k</font></code><code><sub>Nk </sub></code></b><code><b>(</b><b><font face="Symbol">l</font></b><b><sub>k,a


</sub></b><b><font face="Symbol">z</font></b><b><sub>k</sub></b><b> + </b><b><font face="Symbol">l</font></b><b><sub>kb</sub></b><b>)</b></code></font></td>
				</tr>
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><b><code><font face="Symbol" size="+1">d</font><font size="+1">L/</font><font face="Symbol" size="+1">d</font><font size="+1">c<sub>k</sub></font></code></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><font size="+1"><code><b><font face="Symbol">S</font></b><b><sub>i=0</sub></b><b><sup>m-1</sup></b><b>


(-2x</b><b><sub>ki</sub></b><b><sup>2</sup></b><b>/</b><b><font face="Symbol">s</font></b><b><sub>ki</sub></b><b><sup>2</sup></b><b>)(y</b><b><sub>ki</sub></b><b>-a</b><b><sub>k</sub></b><b>-b</b><b><sub>k</sub></b><b>x</b><b><sub>ki</sub></b><b>-c</b><b><sub>k</sub></b><b>x</b><b><sub>ki</sub></b><b><sup>2</sup></b><b>-d</b><b><sub>k</sub></b><b>x</b><b><sub>ki</sub></b><b><sup>3</sup></b><b>)


+ </b></code><b><code><font face="Symbol">k</font></code><code><sub>1k </sub></code></b><code><b>(</b><b><font face="Symbol">l</font></b><b><sub>k-1,a


</sub></b><b><font face="Symbol">z</font></b><b><sub>k-1</sub></b><b><sup>2</sup></b><b>


+ 2</b><b><font face="Symbol">l</font></b><b><sub>k-1,b</sub></b><b><font face="Symbol">z</font></b><b><sub>k-1


</sub></b><b>+ </b><b><font face="Symbol">l</font></b><b><sub>k-1,c</sub></b><b>)</b></code></font></td>
				</tr>
				<tr>
					<td></td>
					<td></td>
					<td></td>
					<td><b><code><font size="+1">-</font></code></b></td>
					<td><font size="+1"><b><code><font face="Symbol">k</font></code><code><sub>Nk


</sub></code></b><code><b>(</b><b><font face="Symbol">l</font></b><b><sub>ka </sub></b><b><font face="Symbol">z</font></b><b><sub>k</sub></b><b><sup>2</sup></b><b>


+ 2</b><b><font face="Symbol">l</font></b><b><sub>kb</sub></b><b><font face="Symbol">z</font></b><b><sub>k


</sub></b><b>+ </b><b><font face="Symbol">l</font></b><b><sub>kc</sub></b><b>)</b></code></font></td>
				</tr>
				<tr>
					<td><font size="+1">0</font></td>
					<td><font size="+1">=</font></td>
					<td><b><code><font face="Symbol" size="+1">d</font><font size="+1">L/</font><font face="Symbol" size="+1">d</font><font size="+1">d<sub>k</sub></font></code></b></td>
					<td><font size="+1">=</font></td>
					<td><code><b><font face="Symbol" size="+1">S</font><font size="+1"><sub>i=0</sub><sup>m-1</sup>


(-2x<sub>ki</sub><sup>3</sup>/</font><font face="Symbol" size="+1">s</font><font size="+1"><sub>ki</sub><sup>2</sup>)(y<sub>ki</sub>-a<sub>k</sub>-b<sub>k</sub>x<sub>ki</sub>-c<sub>k</sub>x<sub>ki</sub><sup>2</sup>-d<sub>k</sub>x<sub>ki</sub><sup>3</sup>)</font></b></code></td>
				</tr>
				<tr>
					<td></td>
					<td></td>
					<td></td>
					<td><font size="+1">+</font></td>
					<td><font size="+1"><b><code><font face="Symbol">k</font></code><code><sub>1k


</sub></code></b><code><b>(</b><b><font face="Symbol">l</font></b><b><sub>k-1,a </sub></b><b><font face="Symbol">z</font></b><b><sub>k-1</sub></b><b><sup>3</sup></b><b>


+ 3</b><b><font face="Symbol">l</font></b><b><sub>k-1,b</sub></b><b><font face="Symbol">z</font></b><b><sub>k-1</sub></b><b><sup>2</sup></b><b><sub>


</sub></b><b>+ 3</b><b><font face="Symbol">l</font></b><b><sub>k-1,c</sub></b><b><font face="Symbol">z</font></b><b><sub>k-1</sub></b><b>)</b></code></font></td>
				</tr>
				<tr>
					<td></td>
					<td></td>
					<td></td>
					<td><font size="+1">-</font></td>
					<td><font size="+1"><b><code><font face="Symbol">k</font></code><code><sub>Nk


</sub></code></b><code><b>(</b><b><font face="Symbol">l</font></b><b><sub>ka </sub></b><b><font face="Symbol">z</font></b><b><sub>k</sub></b><b><sup>3</sup></b><b>


+ 3</b><b><font face="Symbol">l</font></b><b><sub>kb</sub></b><b><font face="Symbol">z</font></b><b><sub>k</sub></b><b><sup>2</sup></b><b><sub>


</sub></b><b>+ 3</b><b><font face="Symbol">l</font></b><b><sub>kc</sub></b><b><font face="Symbol">z</font></b><b><sub>k</sub></b><b>)</b></code></font></td>
				</tr>
			</table>
		</div>
		<div align="left">
			<div align="left">
				<p>All this seems awfully complicate.&nbsp;It is not. Let us define:</p>
			</div>
		</div>
		<div align="center">
			<table border="1" cellpadding="2" cellspacing="1" bgcolor="white">
				<tr>
					<td><b><font size="+1"><code><font face="Symbol">a</font><sub>k,mn</sub></code></font></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><code><b><font face="Symbol" size="+1">S</font><font size="+1"><sub>i=0</sub><sup>m-1</sup>


(2x<sub>ki</sub><sup>m+n</sup>/</font><font face="Symbol" size="+1">s</font><font size="+1"><sub>ki</sub><sup>2</sup>)</font></b></code></td>
					<td><code><b><font size="+1">m=0..3, n=0..3</font></b></code></td>
				</tr>
				<tr>
					<td><b><font size="+1"><code><font face="Symbol">b</font><sub>k,m</sub></code></font></b></td>
					<td><b><code><font size="+1">=</font></code></b></td>
					<td><code><b><font face="Symbol" size="+1">S</font><font size="+1"><sub>i=0</sub><sup>m-1</sup>


(2x<sub>ki</sub><sup>m </sup>y<sub>ki</sub>/</font><font face="Symbol" size="+1">s</font><font size="+1"><sub>ki</sub><sup>2</sup>)</font></b></code></td>
					<td><code><b><font size="+1">k=1..N</font></b></code></td>
				</tr>
			</table>
		</div>
		<div align="left">
			<div align="left">
				<p>The miracle is that this system of <b>7*N</b> equations can be written in


matrix form, and that the appearing matrix has the very nice property of being a


band matrix! Let us define the following vector <b><code><font color="#006600" size="+2">x</font></code></b>


(vector of the unknowns) of dimension <b>7*N</b>: (vectors are in green, matrices


in red and underlined)</p>
			</div>
		</div>
		<div align="center">
			<table border="1" cellpadding="2" cellspacing="1" bgcolor="white">
				<tr>
					<td rowspan="15"><b><code><font color="#006600" size="+2">x</font></code></b></td>
					<td rowspan="15"><b><code><font size="+1">=</font></code></b></td>
					<td><code><font size="+1"><b>a<sub>1</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b>b<sub>1</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b>c<sub>1</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b>d<sub>1</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b><font face="Symbol">l</font><sub>1a</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b><font face="Symbol">l</font><sub>1b</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b><font face="Symbol">l</font><sub>1c</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b>a<sub>2</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b>b<sub>2</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b>c<sub>2</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b>d<sub>2</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b><font face="Symbol">l</font><sub>2a</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b><font face="Symbol">l</font><sub>2b</sub></b></font></code></td>
				</tr>
				<tr>
					<td><code><font size="+1"><b><font face="Symbol">l</font><sub>2c</sub></b></font></code></td>
				</tr>
				<tr>
					<td><b><code><font size="+1">*</font></code></b>
						<p><b><code><font size="+1">*</font></code></b></p>
						<p><b><code><font size="+1">*</font></code></b></p>
					</td>
				</tr>
			</table>
		</div>
		<div align="left">
			<div align="left">
				<p>Let us also define the following vector <b><code><font color="#006600" size="+1">b</font></code></b>


(right-hand side vector) of dimension 7*N:</p>
				<p></p>
			</div>
		</div>
		<div align="center">
			<table border="1" cellpadding="2" cellspacing="1" bgcolor="white">
				<tr>
					<td rowspan="15"><b><code><font color="#006600" size="+2">b</font></code></b></td>
					<td rowspan="15"><b><code><font size="+1">=</font></code></b></td>
					<td><b><code><font face="Symbol" size="+1">b</font><font size="+1"><sub>1,0</sub></font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font face="Symbol" size="+1">b</font><font size="+1"><sub>1,1</sub></font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font face="Symbol" size="+1">b</font><font size="+1"><sub>1,2</sub></font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font face="Symbol" size="+1">b</font><font size="+1"><sub>1,3</sub></font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font face="Symbol" size="+1">b</font><font size="+1"><sub>2,0</sub></font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font face="Symbol" size="+1">b</font><font size="+1"><sub>2,1</sub></font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font face="Symbol" size="+1">b</font><font size="+1"><sub>2,2</sub></font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font face="Symbol" size="+1">b</font><font size="+1"><sub>2,3</sub></font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td><b><code><font size="+1">*</font></code></b>
						<p><b><code><font size="+1">*</font></code></b></p>
						<p><b><code><font size="+1">*</font></code></b></p>
					</td>
				</tr>
			</table>
		</div>
		<div align="left">
			<div align="left">
				<p>and the following matrix, of dimension <b><code>(7*N)*(7*N)</code></b>:</p>
				<p></p>
			</div>
		</div>
		<div align="center">
			<table border="1" cellpadding="1" cellspacing="1" bgcolor="white">
				<tr>
					<td rowspan="22" align="center"><b><code><font color="#cc0000"><u>A</u></font></code></b></td>
					<td rowspan="22" align="center"><b><code>=</code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>1,00</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,01</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,02</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,03</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,10</sub></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>1,11</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,12</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,13</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>1</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,20</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,21</sub></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>1,22</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,23</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>1</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-2<font face="Symbol">z</font><sub>1</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,30</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,31</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>1,32</sub></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>1,33</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>1</sub><sup>3</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-3<font face="Symbol">z</font><sub>1</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-3<font face="Symbol">z</font><sub>1</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>1</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>1</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>1</sub><sup>3</sup></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><b><font face="Symbol">z</font></b><b><sub>1</sub></b></code></td>
					<td bgcolor="#ffffcc" align="center"><code><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>2</sup></b></code></td>
					<td bgcolor="#ffffcc" align="center"><code><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>3</sup></b></code></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><b>-2</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b></code></td>
					<td bgcolor="#ffffcc" align="center"><code><b>-3</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>2</sup></b></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><b>2</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b></code></td>
					<td bgcolor="#ffffcc" align="center"><code><b>3</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>2</sup></b></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-3<font face="Symbol">z</font><sub>1</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><b>3</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>2,00</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,01</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,02</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,03</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">z</font><sub>1</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,10</sub></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>2,11</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,12</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,13</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>2</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">z</font><sub>1</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>2<font face="Symbol">z</font><sub>1</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,20</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,21</sub></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>2,22</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,23</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>2</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-2<font face="Symbol">z</font><sub>2</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">z</font><sub>1</sub><sup>3</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>3<font face="Symbol">z</font><sub>1</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>3<font face="Symbol">z</font><sub>1</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,30</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,31</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,32</sub></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>2,33</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>2</sub><sup>3</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-3<font face="Symbol">z</font><sub>2</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-3<font face="Symbol">z</font><sub>2</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>2</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>2</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>2</sub><sup>3</sup></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><b><font face="Symbol">z</font></b><b><sub>2</sub></b></code></td>
					<td bgcolor="#ffffcc" align="center"><code><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>2</sup></b></code></td>
					<td bgcolor="#ffffcc" align="center"><code><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>3</sup></b></code></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><b>-2</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b></code></td>
					<td bgcolor="#ffffcc" align="center"><code><b>-3</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>2</sup></b></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><b>2</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b></code></td>
					<td bgcolor="#ffffcc" align="center"><code><b>3</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>2</sup></b></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-3<font face="Symbol">z</font><sub>2</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><b>3</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>3,00</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>3,01</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,02</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>3,03</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">z</font><sub>2</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>3,10</sub></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>3,11</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>3,12</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>3,13</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>3</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">z</font><sub>2</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>2<font face="Symbol">z</font><sub>2</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>3,20</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>3,21</sub></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>3,22</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>3,23</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>3</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-2<font face="Symbol">z</font><sub>3</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">z</font><sub>2</sub><sup>3</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>3<font face="Symbol">z</font><sub>2</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>3<font face="Symbol">z</font><sub>2</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,30</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,31</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol">a</font><sub>2,32</sub></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol">a</font><sub>2,33</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>3</sub><sup>3</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-3<font face="Symbol">z</font><sub>3</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-3<font face="Symbol">z</font><sub>3</sub></code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>3</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>3</sub><sup>2</sup></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-<font face="Symbol">z</font><sub>3</sub><sup>3</sup></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><b>-2</b><b><font face="Symbol">z</font></b><b><sub>3</sub></b></code></td>
					<td bgcolor="#ffffcc" align="center"><code><b>-3</b><b><font face="Symbol">z</font></b><b><sub>3</sub></b><b><sup>2</sup></b></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-1</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>-3<font face="Symbol">z</font><sub>3</sub></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code>0</code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code>0</code></b></td>
					<td align="center"><b><code>***</code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
					<td align="center"><b><code>*</code></b>
						<p><b><code>*</code></b></p>
						<p><b><code>*</code></b></p>
					</td>
				</tr>
			</table>
		</div>
		<div align="left">
			<p></p>
			<p><b><code><font size="+2" color="#cc0000"><u>A</u></font></code></b> is a band


matrix with <b><code>M<sub>b</sub> = 6</code></b> (width of the band = <b><code>2M<sub>b</sub>


+&nbsp;1</code></b>). The problem can be written:</p>
		</div>
		<div align="center">
			<table border="1" cellpadding="2" cellspacing="1" bgcolor="white">
				<tr>
					<td><b><code><font size="+2" color="#cc0000"><u>A</u></font></code></b></td>
					<td>*</td>
					<td><b><code><font color="#006600" size="+2">x</font></code></b></td>
					<td>=</td>
					<td><b><code><font color="#006600" size="+2">b</font></code></b></td>
				</tr>
			</table>
		</div>
		<div align="left">
			<p>Let us write matrix A in case of N=2 or 3 intervals.&nbsp;It will be useful


to understand the content of the last lines and columns!</p>
			<p></p>
			<p></p>
		</div>
		<div align="center">
			<table border="1" cellpadding="2" cellspacing="1" bgcolor="white">
				<caption>
					<p><code><b>case N=2.&nbsp;In this case : number of lines 7N-3 = 11, number


of columns of compact matrix = 11 &lt;&gt; 2M<sub>b</sub> +&nbsp;1 = 13</b></code></p>
				</caption>
				<tr>
					<td rowspan="11" align="center"><b><code><font color="#cc0000" size="+2"><u>A</u></font></code></b></td>
					<td rowspan="11" align="center"><b><code><font size="+1">=</font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,00</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,01</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,02</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,03</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,10</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,11</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,12</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,13</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,20</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,21</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,22</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,23</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,30</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,31</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,32</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,33</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font><sub>1</sub><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font><sub>1</sub><sup>3</sup></b></font></code></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-2<font face="Symbol">z</font><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-3<font face="Symbol">z</font><sub>1</sub><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>2<font face="Symbol">z</font><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3<font face="Symbol">z</font><sub>1</sub><sup>2</sup></b></font></code></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3<font face="Symbol">z</font><sub>1</sub></b></font></code></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,00</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,01</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,02</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,03</sub></font></code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,10</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,11</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,12</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,13</sub></font></code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,20</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,21</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,22</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,23</sub></font></code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,30</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,31</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,32</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,33</sub></font></code></b></td>
				</tr>
			</table>
			<p></p>
			<table border="1" cellpadding="2" cellspacing="1" bgcolor="white">
				<caption>
					<p><code><b>case N=2 in compact form.&nbsp;See TBandedLE to know what the compact


form is</b></code></p>
				</caption>
				<tr>
					<td rowspan="11" align="center"><b><code><font color="#cc0000" size="+2"><u>A</u></font></code></b></td>
					<td rowspan="11" align="center"><b><code><font size="+1">=</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,00</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,01</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,02</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,03</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,10</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,11</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,12</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,13</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,20</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,21</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,22</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,23</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,30</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,31</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,32</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,33</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font><sub>1</sub><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font><sub>1</sub><sup>3</sup></b></font></code></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-2<font face="Symbol">z</font><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-3<font face="Symbol">z</font><sub>1</sub><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>2<font face="Symbol">z</font><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3<font face="Symbol">z</font><sub>1</sub><sup>2</sup></b></font></code></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3<font face="Symbol">z</font><sub>1</sub></b></font></code></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,00</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,01</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,02</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,03</sub></font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,10</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,11</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,12</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,13</sub></font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,20</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,21</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,22</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,23</sub></font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,30</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,31</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,32</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,33</sub></font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
			</table>
			<p></p>
			<p></p>
			<table border="1" cellpadding="2" cellspacing="1" bgcolor="white">
				<caption><code><b>case N=3.&nbsp;In this case : number of lines 7N-3 = 18, number


of columns of compact matrix 2M<sub>b</sub> +&nbsp;1 = 13</b></code></caption>
				<tr>
					<td rowspan="18" align="center"><b><code><font color="#cc0000" size="+1"><u>A</u></font></code></b></td>
					<td rowspan="18" align="center"><b><code><font size="+1">=</font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,00</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,01</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,02</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,03</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,10</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,11</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,12</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,13</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,20</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,21</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,22</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,23</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,30</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,31</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,32</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,33</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>3</sup></b></font></code></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-2</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-3</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>2</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,00</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,01</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,02</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,03</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,10</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,11</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,12</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,13</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,20</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,21</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,22</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,23</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,30</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,31</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,32</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,33</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>2</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>3</sup></b></font></code></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-2</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-3</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>2</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>2</sup></b></font></code></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b></font></code></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,00</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,01</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,02</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,03</sub></font></code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,10</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,11</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,12</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,13</sub></font></code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,20</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,21</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,22</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,23</sub></font></code></b></td>
				</tr>
				<tr>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,30</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,31</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,32</sub></font></code></b></td>
					<td bgcolor="#ffff66" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,33</sub></font></code></b></td>
				</tr>
			</table>
			<p></p>
			<table border="1" cellpadding="2" cellspacing="1" bgcolor="white">
				<caption><code><b>case N=3 in compact form See TBandedLE to know what the compact


form is</b></code></caption>
				<tr>
					<td rowspan="18" align="center"><b><code><font color="#cc0000" size="+1"><u>A</u></font></code></b></td>
					<td rowspan="18" align="center"><b><code><font size="+1">=</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,00</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,01</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,02</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,03</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,10</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,11</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,12</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,13</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,20</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,21</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,22</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,23</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,30</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,31</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,32</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>1,33</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>3</sup></b></font></code></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-2</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-3</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>2</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b><b><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3</b><b><font face="Symbol">z</font></b><b><sub>1</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,00</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,01</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,02</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,03</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,10</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,11</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,12</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,13</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,20</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,21</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,22</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,23</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>1</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,30</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,31</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,32</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,33</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"></td>
					<td bgcolor="#ffffcc" align="center"></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>2</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>3</sup></b></font></code></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-2</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>-3</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>2</sup></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>2</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b></font></code></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b><b><sup>2</sup></b></font></code></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">-3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><code><font size="+1"><b>3</b><b><font face="Symbol">z</font></b><b><sub>2</sub></b></font></code></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,00</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,01</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,02</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,03</sub></font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,10</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,11</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,12</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,13</sub></font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td align="center" bgcolor="#ffffcc"><b><code><font size="+1">0</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">2</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">1</font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,20</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,21</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,22</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>3,23</sub></font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
				<tr>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>3</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub><sup>2</sup></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font size="+1">3</font><font face="Symbol" size="+1">z</font><font size="+1"><sub>2</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,30</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,31</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,32</sub></font></code></b></td>
					<td bgcolor="#ffffcc" align="center"><b><code><font face="Symbol" size="+1">a</font><font size="+1"><sub>2,33</sub></font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
					<td align="center"><b><code><font size="+1">0</font></code></b></td>
				</tr>
			</table>
		</div>
		<div align="left">
			<p></p>
			<p>In its compact form as required by TBandedLE, matrix <b><code><font color="#cc0000" size="+1"><u>A</u></font></code></b>


will have the dimension (7N-3,13) except for the special case N=2 for which the dimensions


of the compact matrix will be (7N-3,11).</p>
			<p></p>
		</div>
		<div align="center">
			<h2></h2>
		</div>
<pre>
<!--*/
// -->END_HTML
//
TSplineFit::TSplineFit() {
//
// 1ST constructor. Default constructor
//
  Init();
}
TSplineFit::TSplineFit(Text_t *name,Text_t *title,Int_t cat,Int_t M, Int_t m,
  Double_t *x,Double_t *y,Double_t *sig,Bool_t lowbounded,Double_t ylow,Bool_t upbounded,
  Double_t yup,Double_t xmin,Double_t xmax,Bool_t debug):TNamed(name,title) {
//
// 2ND constructor. 1D Spline fit [m>1] or Spline interpolation [m==1].
//                  Measurements provided in arrays.
//
// Constructor giving the measurements. TSplineFit has name and title. It is because TSplineFit
//is intended to be used for instance for representing variable parameters inside a Monte-Carlo.
//In that case, all the variable parameters will be stored into the collection fgFits. We want
//the variable parameters to be retrievable by their name and it is why we assign a name and a
//title to TSplineFit.
// These variable parameters may also belong to different categories, it is also why we include
//an arbitrary category "cat" among the parameters. To increase the speed of the search in fgFits
//when the category of the searched TSplineFit is known, we have at our disposal the static
//TArrayI fgCat, fgCat[i] being the position in fgFits of the first element of category i.
//The categories must be less than 99.
// For instance, in a Monte-Carlo like Litrani, variable parameters may belong to the following
//categories:
//   - index of refraction of materials                    (category 1)
//   - Element of dielectric tensor                        (category 2)
//   - Real part of index of refraction of revetments      (category 3)
//   - Imaginary part of index of refraction of revetments (category 4)
//   - ..............................................
//   - Gain profile of an APD                              (category 13)
//   - and so on...
// We impose also the following restriction concerning name and title:
//if you are entering the first element of the category i,
//   - the name must have 2 parts, separated by '_':
//       -- Before '_', give a name for the category
//       -- After  '_', give a name for the fit
//   - the title must have 2 parts, separated by " | ":
//       -- Before " | ", give a more detailed explanation for the category
//       -- After  " | ", give a more detailed explanation for the fit
//if you are entering following elements of the same category i,
//       --it is no more necessary to give the part preceeding '_' in the name, it will be
//          prepended.
//       --it is no more necessary to give the part preceeding " | " in the title, it will be
//          prepended.
// As an example, suppose your program needs the imaginary part of the index of refraction
//of aluminium and of mylar. You could decide to give category number 3 for imaginary
//part of index of refraction of wrappings and call the 2 TSplineFit like this:
//
//  TSplineFit *imalu   = new TSplineFit("ImIndex_Aluminium",
//              "Imaginary part of refraction index | Standard aluminium",3,...);
//  TSplineFit *immylar = new TSplineFit("Mylar","mylar from 3M",3,...);
//
// The x extension of the fit is between xmin and xmax. If xmin and xmax are not given,
//or xmin and xmax have unacceptable values, the following values for xmin and xmax are
//taken (x[0] and x[1] are the x of the first 2 measurements, x[m-2] and x[m-1] are the x
//of the last 2 measurements) :
//
//   xmin = (3*x[0]   - x[1])/2
//   xmax = (3*x[m-1] - x[m-2])/2
//
// The possibility exists to impose a lower bound ylow for the fitted value y. Look at the
//arguments lowbounded and ylow. This is interesting for instance in case you fit a value
//for which a negative value does not make sense. Or for instance when fitting an index of
//refraction it would not make sense to get a value smaller than 1. This is obtained by
//fitting, not the y[i] values, but the yn[i] values, where:
//   yn[i] = y[i] - ylow - 1/(y[i] - ylow)
// The reverse transformation is then applied when giving the result.
// If a lower bound is requested and if one of the value y[i] is below the bound, it will
//be modified and set slightly above the bound. A warning will be issued.
// The possibility also exists to set an upper bound. See the arguments upbounded and
//yup. But here the fit is NOT modified and CAN give values above yup. It is only when
//calling method V() that if the fitted value is above yup it is replaced by yup! Be
//aware of the difference between lower and upper bound.
//
//   Arguments:
//
// name        : [no default]    name of this spline fit
// title       : [no default]    title of this spline fit
// cat         : [no default]    category to which this spline fit belongs (arbitrary)
// M           : [no default]    number of measurements
// m           : [no default]    number of measurements per spline.
// x           : [no default]    list of abscissa values of measurements in increasing order
// y           : [no default]    list of values of y[i] measurements at x[i]
// sig         : [default 0.0]   errors on y[i] measurements at x[i]. If sig==0, then
//                                all errors assumed to be 1.0.
// lowbounded  : [default false] y value have a lower bound. The fit is not allowed to go
//                                below this bound
// ylow        : [default 0.0]   lower bound for y. [irrelevant if lowbounded false]
// upbounded   : [default false] value have an upper bound. The value returned by V() is
//                                cut at yup
// yup         : [default  1.0]  upper bound for y. [irrelevant if upbounded false]
// xmin        : [default  1.0]  lower bound in x for the display of the fit.
// xmax        : [default -1.0]  upper bound for the display of the fit. The default
//                                for xmin and xmax are set in such a way that the
//                                constructor will recalculate them as indicated above.
// debug       : [default false] if true, print matrix and vector of the problem and quality of solution.
//
  const Double_t un = 1.0;
  Bool_t ok;
  Int_t i,ifail;
  Init(); FindDate();
  ok = InitCatAndBounds(cat,lowbounded,ylow,upbounded,yup);
  VerifyNT();
  InitDimensions(M,m);
//Registering data values
  for (i=0;i<fM;i++) {
    fMt[i] = x[i];
    fMv[i] = y[i];
    if (sig==0) fMs[i] = un;
    else        fMs[i] = sig[i];
  }
  ok = InitCheckBounds();
  InitIntervals(xmin,xmax);
  if (fMi<=1) {
    fType = SplineInterpol;
    ifail = Interpolation();
  }
  else {
    fType = SplineFit1D;
    Fill();
    ifail = Solve(debug);
  }
  if (ifail) {
    std::cout << "TSplineFit::TSplineFit Problem in finding solution" << std::endl;
    std::cout << "   ifail = " << ifail << std::endl;
  }
  else  {
    AddThisFit();
    if (gSplineFit) gSplineFit->CutLinkWithHisto();
    gSplineFit = this;
  }
  fA.ResizeTo(1,1);
  fB.ResizeTo(1,1);
}
TSplineFit::TSplineFit(Text_t *name,Text_t *title,Int_t cat,Int_t M,
  Double_t *x,Double_t *y,Double_t xmin,Double_t xmax):TNamed(name,title) {
//
// 3RD constructor. 1D linear interpolation
//                  Measurements provided in arrays.
//
// There are cases where a simple linear interpolation is preferable. This is for instance
//the case for the gain profile of an APD, which has very long linear portions.
// Use this constructor ONLY if all you want is simply a linear interpolation between the
//measured data points. In that case, no possibility of low or up bound.
// Constructor giving the measurements. TSplineFit has name and title. It is because TSplineFit
//is intended to be used for instance for representing variable parameters inside a Monte-Carlo.
//In that case, all the variable parameters will be stored into the collection fgFits. We want
//the variable parameters to be retrievable by their name and it is why we assign a name and a
//title to TSplineFit.
// These variable parameters may also belong to different categories, it is also why we include
//an arbitrary category "cat" among the parameters. To increase the speed of the search in fgFits
//when the category of the searched TSplineFit is known, we have at our disposal the static
//TArrayI fgCat, fgCat[i] being the position in fgFits of the first element of category i.
//The categories must be less than 99.
// For instance, in a Monte-Carlo like Litrani, variable parameters may belong to the following
//categories:
//   - index of refraction of materials                    (category 1)
//   - Element of dielectric tensor                        (category 2)
//   - Real part of index of refraction of revetments      (category 3)
//   - Imaginary part of index of refraction of revetments (category 4)
//   - ..............................................
//   - Gain profile of an APD                              (category 13)
//   - and so on...
// We impose also the following restriction concerning name and title:
//if you are entering the first element of the category i,
//   - the name must have 2 parts, separated by '_':
//       -- Before '_', give a name for the category
//       -- After  '_', give a name for the fit
//   - the title must have 2 parts, separated by " | ":
//       -- Before " | ", give a more detailed explanation for the category
//       -- After  " | ", give a more detailed explanation for the fit
//if you are entering following elements of the same category i,
//       --it is no more necessary to give the part preceeding '_' in the name, it will be
//          prepended.
//       --it is no more necessary to give the part preceeding " | " in the title, it will be
//          prepended.
// As an example, suppose your program needs the imaginary part of the index of refraction
//of aluminium and of mylar. You could decide to give category number 3 for imaginary
//part of index of refraction of wrappings and call the 2 TSplineFit like this:
//
//  TSplineFit *imalu   = new TSplineFit("ImIndex_Aluminium",
//              "Imaginary part of refraction index | Standard aluminium",3,...);
//  TSplineFit *immylar = new TSplineFit("Mylar","mylar from 3M",3,...);
//
// The x extension of the fit is between xmin and xmax. If xmin and xmax are not given,
//or xmin and xmax have unacceptable values, the following values for xmin and xmax are
//taken (x[0] and x[1] are the x of the first 2 measurements, x[m-2] and x[m-1] are the x
//of the last 2 measurements) :
//
//   xmin = (3*x[0]   - x[1])/2
//   xmax = (3*x[m-1] - x[m-2])/2
//
//
//   Arguments:
//
// name        : [no default]    name of this spline fit
// title       : [no default]    title of this spline fit
// cat         : [no default]    category to which this spline fit belongs (arbitrary)
// M           : [no default]    number of measurements
// x           : [no default]    list of abscissa values of measurements in increasing order
// y           : [no default]    list of values of y[i] measurements at x[i]
// xmin        : [default  1.0]  lower bound in x for the display of the fit.
// xmax        : [default -1.0]  upper bound for the display of the fit. The default
//                                for xmin and xmax are set in such a way that the
//                                constructor will recalculate them as indicated above.
//
  const Double_t dix = 10.0;
  Bool_t ok;
  Int_t i;
  Double_t sizeord = 0.0;
  Init(); FindDate();
  ok = InitCatAndBounds(cat,kFALSE,0.0,kFALSE,1.0);
  VerifyNT();
  InitDimensions(M,1);
  fType = LinInterpol;
//Registering data values
  for (i=0;i<fM;i++)
    if (TMath::Abs(y[i])>sizeord) sizeord = TMath::Abs(y[i]);
  for (i=0;i<fM;i++) {
    fMt[i] = x[i];
    fMv[i] = y[i];
    fMs[i] = sizeord/dix;
  }
  InitIntervals(xmin,xmax);
  AddThisFit();
  if (gSplineFit) gSplineFit->CutLinkWithHisto();
  gSplineFit = this;
  fA.ResizeTo(1,1);
  fB.ResizeTo(1,1);
}
TSplineFit::TSplineFit(Text_t *name,Text_t *title,Int_t cat,Int_t m,
  TGraphErrors *graph,Bool_t lowbounded,Double_t ylow,Bool_t upbounded,
  Double_t yup,Double_t xmin,Double_t xmax,Bool_t debug):TNamed(name,title) {
//
// 4TH constructor. 1D Spline fit [m>1] or Spline interpolation [m==1].
//                  Measurements provided in a TGraphErrors
//
// Constructor taking the measurements from a TGraphErrors. If xmin>=xmax [default] all
//points of the TGraphErrors graph are taken and reasonable values for xmin and xmax are
//recalculated. If xmin<xmax, only the points between xmin and xmax are taken.
// TSplineFit has name and title. It is because TSplineFit is intended to be used for instance
//for representing variable parameters inside a Monte-Carlo. In that case, all the variable
//parameters will be stored into the collection fgFits. We want the variable parameters to be
//retrievable by their name and it is why we assign a name and a title to TSplineFit.
// These variable parameters may also belong to different categories, it is also why we include
//an arbitrary category "cat" among the parameters. To increase the speed of the search in fgFits
//when the category of the searched TSplineFit is known, we have at our disposal the static
//TArrayI fgCat, fgCat[i] being the position in fgFits of the first element of category i.
//The categories must be less than 99.
// For instance, in a Monte-Carlo like Litrani, variable parameters may belong to the following
//categories:
//   - index of refraction of materials                    (category 1)
//   - Element of dielectric tensor                        (category 2)
//   - Real part of index of refraction of revetments      (category 3)
//   - Imaginary part of index of refraction of revetments (category 4)
//   - ..............................................
//   - Gain profile of an APD                              (category 13)
//   - and so on...
// We impose also the following restriction concerning name and title:
//if you are entering the first element of the category i,
//   - the name must have 2 parts, separated by '_':
//       -- Before '_', give a name for the category
//       -- After  '_', give a name for the fit
//   - the title must have 2 parts, separated by " | ":
//       -- Before " | ", give a more detailed explanation for the category
//       -- After  " | ", give a more detailed explanation for the fit
//if you are entering following elements of the same category i,
//       --it is no more necessary to give the part preceeding '_' in the name, it will be
//          prepended.
//       --it is no more necessary to give the part preceeding " | " in the title, it will be
//          prepended.
// As an example, suppose your program needs the imaginary part of the index of refraction
//of aluminium and of mylar. You could decide to give category number 3 for imaginary
//part of index of refraction of wrappings and call the 2 TSplineFit like this:
//
//  TSplineFit *imalu   = new TSplineFit("ImIndex_Aluminium",
//              "Imaginary part of refraction index | Standard aluminium",3,...);
//  TSplineFit *immylar = new TSplineFit("Mylar","mylar from 3M",3,...);
//
// The x extension of the fit is between xmin and xmax. If xmin and xmax are not given,
//or xmin and xmax have unacceptable values, the following values for xmin and xmax are
//taken (x[0] and x[1] are the x of the first 2 measurements, x[m-2] and x[m-1] are the x
//of the last 2 measurements) :
//
//   xmin = (3*x[0]   - x[1])/2
//   xmax = (3*x[m-1] - x[m-2])/2
//
// The possibility exists to impose a lower bound ylow for the fitted value y. Look at the
//arguments lowbounded and ylow. This is interesting for instance in case you fit a value
//for which a negative value does not make sense. Or for instance when fitting an index of
//refraction it would not make sense to get a value smaller than 1. This is obtained by
//fitting, not the y[i] values, but the yn[i] values, where:
//   yn[i] = y[i] - ylow - 1/(y[i] - ylow)
// The reverse transformation is then applied when giving the result.
// If a lower bound is requested and if one of the value y[i] is below the bound, it will
//be modified and set slightly above the bound. A warning will be issued.
// The possibility also exists to set an upper bound. See the arguments upbounded and
//yup. But here the fit is NOT modified and CAN give values above yup. It is only when
//calling method V() that if the fitted value is above yup it is replaced by yup! Be
//aware of the difference between lower and upper bound.
//
//   Arguments:
//
// name        : [no default]    name of this spline fit
// title       : [no default]    title of this spline fit
// cat         : [no default]    category to which this spline fit belongs (arbitrary)
// m           : [no default]    number of measurements per spline.
// graph       : [no default]    graph from which to get measurements x,y,sig
// lowbounded  : [default false] y value have a lower bound. The fit is not allowed to go
//                                below this bound
// ylow        : [default 0.0]   lower bound for y. [irrelevant if lowbounded false]
// upbounded   : [default false] value have an upper bound. The value returned by V() is
//                                cut at yup
// yup         : [default  1.0]  upper bound for y. [irrelevant if upbounded false]
// xmin        : [default  1.0]  lower bound in x for the display of the fit.
// xmax        : [default -1.0]  upper bound for the display of the fit. The default
//                                for xmin and xmax are set in such a way that the
//                                constructor will recalculate them as indicated above.
// debug       : [default false] if true, print matrix and vector of the problem and quality of solution.
//
  Int_t npoints,M;
  Bool_t ok;
  Int_t i,k,ifail;
  Double_t x,y;
  Init(); FindDate();
  ok = InitCatAndBounds(cat,lowbounded,ylow,upbounded,yup);
  VerifyNT();
  npoints = graph->GetN();
  if (xmin>=xmax) M = npoints;
  else {
    M = 0;
    for (i=0;i<npoints;i++) {
      graph->GetPoint(i,x,y);
      if ((x>=xmin) && (x<=xmax)) M++;
    }
  }
  InitDimensions(M,m);
//Registering data values
  k = 0;
  for (i=0;i<npoints;i++) {
    if (xmin>=xmax) {
      graph->GetPoint(k,fMt[k],fMv[k]);
      fMs[k] = graph->GetErrorY(k);
      k++;
    }
    else {
      graph->GetPoint(i,x,y);
      if ((x>=xmin) && (x<=xmax)) {
        fMt[k] = x;
        fMv[k] = y;
        fMs[k] = graph->GetErrorY(k);
        k++;
      }//end if ((x>=xmin) && (x<=xmax))
    }//end else if (xmin>=xmax)
  }//end for (i=0;i<npoints;i++)
  ok = InitCheckBounds();
  InitIntervals(xmin,xmax);
  if (fMi<=1) {
    fType = SplineInterpol;
    ifail = Interpolation();
  }
  else {
    fType = SplineFit1D;
    Fill();
    ifail = Solve(debug);
  }
  if (ifail) {
    std::cout << "TSplineFit::TSplineFit Problem in finding solution" << std::endl;
    std::cout << "   ifail = " << ifail << std::endl;
  }
  else  {
    AddThisFit();
    if (gSplineFit) gSplineFit->CutLinkWithHisto();
    gSplineFit = this;
  }
  fA.ResizeTo(1,1);
  fB.ResizeTo(1,1);
}
TSplineFit::TSplineFit(Text_t *name,Text_t *title,Int_t cat,Int_t m,
  TH1D *h, Double_t fxmin, Double_t fxmax, Bool_t lowbounded,Double_t ylow,Bool_t upbounded,
  Double_t yup,Bool_t debug):TNamed(name,title) {
//
// 5TH constructor. 1D Spline fit [m>1] or Spline interpolation [m==1].
//                  Measurements provided in an histogram of type TH1D
//
// Constructor taking the measurements from an histogram.
//TSplineFit is nice enough to create a TF1, using the function SplineFitFunc, and to
//add this function to the list of functions of the histogram, so that if you later plot
//the histogram , the spline fit will also be plotted. As an example, having histogram h1,
//you can do :
//
// TSplineFit *sf;
//  sf = new TSplineFit("Divers_Test2","Divers | Test2",0,m,h1);
//  h1->Draw()      or      sf->DrawHisto(kTRUE);
//
// TSplineFit has name and title. It is because TSplineFit is intended to be used for instance
//for representing variable parameters inside a Monte-Carlo. In that case, all the variable
//parameters will be stored into the collection fgFits. We want the variable parameters to be
//retrievable by their name and it is why we assign a name and a title to TSplineFit.
// These variable parameters may also belong to different categories, it is also why we include
//an arbitrary category "cat" among the parameters. To increase the speed of the search in fgFits
//when the category of the searched TSplineFit is known, we have at our disposal the static
//TArrayI fgCat, fgCat[i] being the position in fgFits of the first element of category i.
//The categories must be less than 99.
// For instance, in a Monte-Carlo like Litrani, variable parameters may belong to the following
//categories:
//   - index of refraction of materials                    (category 1)
//   - Element of dielectric tensor                        (category 2)
//   - Real part of index of refraction of revetments      (category 3)
//   - Imaginary part of index of refraction of revetments (category 4)
//   - ..............................................
//   - Gain profile of an APD                              (category 13)
//   - and so on...
// We impose also the following restriction concerning name and title:
//if you are entering the first element of the category i,
//   - the name must have 2 parts, separated by '_':
//       -- Before '_', give a name for the category
//       -- After  '_', give a name for the fit
//   - the title must have 2 parts, separated by " | ":
//       -- Before " | ", give a more detailed explanation for the category
//       -- After  " | ", give a more detailed explanation for the fit
//if you are entering following elements of the same category i,
//       --it is no more necessary to give the part preceeding '_' in the name, it will be
//          prepended.
//       --it is no more necessary to give the part preceeding " | " in the title, it will be
//          prepended.
// As an example, suppose your program needs the imaginary part of the index of refraction
//of aluminium and of mylar. You could decide to give category number 3 for imaginary
//part of index of refraction of wrappings and call the 2 TSplineFit like this:
//
//  TSplineFit *imalu   = new TSplineFit("ImIndex_Aluminium",
//              "Imaginary part of refraction index | Standard aluminium",3,...);
//  TSplineFit *immylar = new TSplineFit("Mylar","mylar from 3M",3,...);
//
// The possibility exists to impose a lower bound ylow for the fitted value y. Look at the
//arguments lowbounded and ylow. This is interesting for instance in case you fit a value
//for which a negative value does not make sense. Or for instance when fitting an index of
//refraction it would not make sense to get a value smaller than 1. This is obtained by
//fitting, not the y[i] values, but the yn[i] values, where:
//   yn[i] = y[i] - ylow - 1/(y[i] - ylow)
// The reverse transformation is then applied when giving the result.
// If a lower bound is requested and if one of the value y[i] is below the bound, it will
//be modified and set slightly above the bound. A warning will be issued.
// The possibility also exists to set an upper bound. See the arguments upbounded and
//yup. But here the fit is NOT modified and CAN give values above yup. It is only when
//calling method V() that if the fitted value is above yup it is replaced by yup! Be
//aware of the difference between lower and upper bound.
//
//   Arguments:
//
// name        : [no default]    name of this spline fit
// title       : [no default]    title of this spline fit
// cat         : [no default]    category to which this spline fit belongs (arbitrary)
// m           : [no default]    number of measurements per spline.
// h           : [no default]    histogram from which to get measurements x,y,sig
// lowbounded  : [default false] y value have a lower bound. The fit is not allowed to go
//                                below this bound
// ylow        : [default 0.0]   lower bound for y. [irrelevant if lowbounded false]
// upbounded   : [default false] value have an upper bound. The value returned by V() is
//                                cut at yup
// yup         : [default  1.0]  upper bound for y. [irrelevant if upbounded false]
// xmin        : [default  1.0]  lower bound in x for the display of the fit.
// xmax        : [default -1.0]  upper bound for the display of the fit. The default
//                                for xmin and xmax are set in such a way that the
//                                constructor will recalculate them as indicated above.
// debug       : [default false] if true, print matrix and vector of the problem and quality of solution.
//
  const Double_t eps = 0.01;
  const TArrayD *xbins;
  Int_t npoints,M;
  Double_t xmin,xmax;
  TAxis *xaxis;
  Bool_t ok;
  Int_t j,ifail;
  Init(); FindDate();
  ok = InitCatAndBounds(cat,lowbounded,ylow,upbounded,yup);
  VerifyNT();
  npoints = h->GetNbinsX();
  xaxis   = h->GetXaxis();
  xmin = fxmin;
  xmax = fxmax;
//  xmin    = xaxis->GetXmin();
//  xmax    = xaxis->GetXmax();
  xbins   = xaxis->GetXbins();
  j       = xbins->fN;
  if (j==0)   fZigZag = new TZigZag(npoints,xmin,xmax);
  //M = npoints;
  M = GetDimensionHist(xmin,xmax,eps,h); //XZ

  if(M==0)std::cout << "TSplineFit::TSplineFit ERROR: found NO data in the histogram!" <<std::endl;
  InitDimensions(M,m);
  GetDataFromHist(xmin,xmax,eps,h);
  ok = InitCheckBounds();
  InitIntervals(xmin,xmax);
  if (fMi<=1) {
    fType = SplineInterpol;
    ifail = Interpolation();
  }
  else {
    fType = SplineFit1D;
    Fill();
    ifail = Solve(debug);
  }
  if (ifail) {
    std::cout << "TSplineFit::TSplineFit Problem in finding solution" << std::endl;
    std::cout << "   ifail = " << ifail << std::endl;
  }
  else  {
    AddThisFit();
    if (gSplineFit) gSplineFit->CutLinkWithHisto();
    gSplineFit = this;
    fSplineFitFunc = new TF1("SplineFitFunc",SplineFitFunc,fKhi[0],fKhi[fN],0);
    TList *list;
    list = h->GetListOfFunctions();
    list->Add(fSplineFitFunc);
    fProvidedH1D = h;
    fProvidedName  = fProvidedH1D->GetName();
  }
  fA.ResizeTo(1,1);
  fB.ResizeTo(1,1);
}
TSplineFit::TSplineFit(TH1D *h,Int_t cat,Int_t m,Bool_t lowbounded,Double_t ylow,
  Bool_t upbounded,Double_t yup,Bool_t debug):TNamed() {
//
// 6TH constructor. 1D Spline fit [m>1] or Spline interpolation [m==1].
//                  Measurements provided in an histogram of type TH1D
//
// This constructor is the same as the preceeding one, except that the name and the
//title of the TSplineFit will be the same as the name and the title of the histogram.
//It implies that for naming the histogram, you have followed the rules about name and
//title for a TSplineFit!
// Constructor taking the measurements from an histogram.
//TSplineFit is nice enough to create a TF1, using the function SplineFitFunc, and to
//add this function to the list of functions of the histogram, so that if you later plot
//the histogram , the spline fit will also be plotted. As an example, having histogram h1,
//you can do :
//
// TSplineFit *sf;
//  sf = new TSplineFit(h1,0,m);
//  h1->Draw()      or      sf->DrawHisto(kTRUE);
//
// TSplineFit has name and title. It is because TSplineFit is intended to be used for instance
//for representing variable parameters inside a Monte-Carlo. In that case, all the variable
//parameters will be stored into the collection fgFits. We want the variable parameters to be
//retrievable by their name and it is why we assign a name and a title to TSplineFit.
// These variable parameters may also belong to different categories, it is also why we include
//an arbitrary category "cat" among the parameters. To increase the speed of the search in fgFits
//when the category of the searched TSplineFit is known, we have at our disposal the static
//TArrayI fgCat, fgCat[i] being the position in fgFits of the first element of category i.
//The categories must be less than 99.
// For instance, in a Monte-Carlo like Litrani, variable parameters may belong to the following
//categories:
//   - index of refraction of materials                    (category 1)
//   - Element of dielectric tensor                        (category 2)
//   - Real part of index of refraction of revetments      (category 3)
//   - Imaginary part of index of refraction of revetments (category 4)
//   - ..............................................
//   - Gain profile of an APD                              (category 13)
//   - and so on...
// We impose also the following restriction concerning name and title:
//if you are entering the first element of the category i,
//   - the name must have 2 parts, separated by '_':
//       -- Before '_', give a name for the category
//       -- After  '_', give a name for the fit
//   - the title must have 2 parts, separated by " | ":
//       -- Before " | ", give a more detailed explanation for the category
//       -- After  " | ", give a more detailed explanation for the fit
//if you are entering following elements of the same category i,
//       --it is no more necessary to give the part preceeding '_' in the name, it will be
//          prepended.
//       --it is no more necessary to give the part preceeding " | " in the title, it will be
//          prepended.
// As an example, suppose your program needs the imaginary part of the index of refraction
//of aluminium and of mylar. You could decide to give category number 3 for imaginary
//part of index of refraction of wrappings and call the 2 TSplineFit like this:
//
//  TSplineFit *imalu   = new TSplineFit("ImIndex_Aluminium",
//              "Imaginary part of refraction index | Standard aluminium",3,...);
//  TSplineFit *immylar = new TSplineFit("Mylar","mylar from 3M",3,...);
//
// The possibility exists to impose a lower bound ylow for the fitted value y. Look at the
//arguments lowbounded and ylow. This is interesting for instance in case you fit a value
//for which a negative value does not make sense. Or for instance when fitting an index of
//refraction it would not make sense to get a value smaller than 1. This is obtained by
//fitting, not the y[i] values, but the yn[i] values, where:
//   yn[i] = y[i] - ylow - 1/(y[i] - ylow)
// The reverse transformation is then applied when giving the result.
// If a lower bound is requested and if one of the value y[i] is below the bound, it will
//be modified and set slightly above the bound. A warning will be issued.
// The possibility also exists to set an upper bound. See the arguments upbounded and
//yup. But here the fit is NOT modified and CAN give values above yup. It is only when
//calling method V() that if the fitted value is above yup it is replaced by yup! Be
//aware of the difference between lower and upper bound.
//
//   Arguments:
//
// h           : [no default]    histogram from which to get measurements x,y,sig
// cat         : [no default]    category to which this spline fit belongs (arbitrary)
// m           : [no default]    number of measurements per spline.
// lowbounded  : [default false] y value have a lower bound. The fit is not allowed to go
//                                below this bound
// ylow        : [default 0.0]   lower bound for y. [irrelevant if lowbounded false]
// upbounded   : [default false] value have an upper bound. The value returned by V() is
//                                cut at yup
// yup         : [default  1.0]  upper bound for y. [irrelevant if upbounded false]
// xmin        : [default  1.0]  lower bound in x for the display of the fit.
// xmax        : [default -1.0]  upper bound for the display of the fit. The default
//                                for xmin and xmax are set in such a way that the
//                                constructor will recalculate them as indicated above.
// debug       : [default false] if true, print matrix and vector of the problem and quality of solution.
//
  const Double_t eps = 0.01;
  const TArrayD *xbins;
  Int_t npoints,M;
  Double_t xmin,xmax;
  TAxis *xaxis;
  Bool_t ok;
  Int_t j,ifail;
  Init(); FindDate();
  SetName(h->GetName());
  SetTitle(h->GetTitle());
  ok = InitCatAndBounds(cat,lowbounded,ylow,upbounded,yup);
  VerifyNT();
  npoints = h->GetNbinsX();
  xaxis   = h->GetXaxis();
  xmin    = xaxis->GetXmin();
  xmax    = xaxis->GetXmax();
  xbins   = xaxis->GetXbins();
  j       = xbins->fN;
  if (j==0)   fZigZag = new TZigZag(npoints,xmin,xmax);
  M = npoints;
  InitDimensions(M,m);
  GetDataFromHist(xmin,xmax,eps,h);
  ok = InitCheckBounds();
  InitIntervals(xmin,xmax);
  if (fMi<=1) {
    fType = SplineInterpol;
    ifail = Interpolation();
  }
  else {
    fType = SplineFit1D;
    Fill();
    ifail = Solve(debug);
  }
  if (ifail) {
    std::cout << "TSplineFit::TSplineFit Problem in finding solution" << std::endl;
    std::cout << "   ifail = " << ifail << std::endl;
  }
  else  {
    AddThisFit();
    if (gSplineFit) gSplineFit->CutLinkWithHisto();
    gSplineFit = this;
    fSplineFitFunc = new TF1("SplineFitFunc",SplineFitFunc,fKhi[0],fKhi[fN],0);
    TList *list;
    list = h->GetListOfFunctions();
    list->Add(fSplineFitFunc);
    fProvidedH1D = h;
    fProvidedName  = fProvidedH1D->GetName();
  }
  fA.ResizeTo(1,1);
  fB.ResizeTo(1,1);
}
TSplineFit::TSplineFit(Text_t *name,Text_t *title,Int_t cat,Int_t m,
  TH2D *h,Bool_t lowbounded,Double_t vlow,Bool_t upbounded,
  Double_t vup,Bool_t debug):TNamed(name,title) {
//
// 7TH constructor. 2D Spline fit. m must be >1. No possibility of Spline interpolation.
//                  Measurements provided in an histogram of type TH2D
//
// Constructor taking the measurements from a 2D histogram. All channels of the histogram
//are taken for the fit. Notice the restriction that:
//  - the points must be equidistant in x and in y, i.e. the 1st constructor of TH2D must have
//      been used:
//      TH2D(const char* name, const char* title, Int_t nbinsx, Axis_t xlow, Axis_t xup,
//           Int_t nbinsy, Axis_t ylow, Axis_t yup)
// The trick explained in the class description of TZigZag is used in order to get a one
//dimensionnal structure from the 2 dimensionnal structure of the histogram. A Spline
//fit in one dimension is then possible. When later on the value of the fit for the point
//(x,y) is asked (by calling V(x,y), the 2 points along the zigzag nearest to (x,y) are
//found by the method TZigZag::PointsNear(). The value returned is then the interpolation
//with respect to y for the 2 points.
// TSplineFit has name and title. It is because TSplineFit is intended to be used for instance
//for representing variable parameters inside a Monte-Carlo. In that case, all the variable
//parameters will be stored into the collection fgFits. We want the variable parameters to be
//retrievable by their name and it is why we assign a name and a title to TSplineFit.
// These variable parameters may also belong to different categories, it is also why we include
//an arbitrary category "cat" among the parameters. To increase the speed of the search in fgFits
//when the category of the searched TSplineFit is known, we have at our disposal the static
//TArrayI fgCat, fgCat[i] being the position in fgFits of the first element of category i.
//The categories must be less than 99.
// For instance, in a Monte-Carlo like Litrani, variable parameters may belong to the following
//categories:
//   - index of refraction of materials                    (category 1)
//   - Element of dielectric tensor                        (category 2)
//   - Real part of index of refraction of revetments      (category 3)
//   - Imaginary part of index of refraction of revetments (category 4)
//   - ..............................................
//   - Gain profile of an APD                              (category 13)
//   - and so on...
// We impose also the following restriction concerning name and title:
//if you are entering the first element of the category i,
//   - the name must have 2 parts, separated by '_':
//       -- Before '_', give a name for the category
//       -- After  '_', give a name for the fit
//   - the title must have 2 parts, separated by " | ":
//       -- Before " | ", give a more detailed explanation for the category
//       -- After  " | ", give a more detailed explanation for the fit
//if you are entering following elements of the same category i,
//       --it is no more necessary to give the part preceeding '_' in the name, it will be
//          prepended.
//       --it is no more necessary to give the part preceeding " | " in the title, it will be
//          prepended.
// As an example, suppose your program needs the imaginary part of the index of refraction
//of aluminium and of mylar. You could decide to give category number 3 for imaginary
//part of index of refraction of wrappings and call the 2 TSplineFit like this:
//
//  TSplineFit *imalu   = new TSplineFit("ImIndex_Aluminium",
//              "Imaginary part of refraction index | Standard aluminium",3,...);
//  TSplineFit *immylar = new TSplineFit("Mylar","mylar from 3M",3,...);
//
// The possibility exists to impose a lower bound ylow for the fitted value y. Look at the
//arguments lowbounded and ylow. This is interesting for instance in case you fit a value
//for which a negative value does not make sense. Or for instance when fitting an index of
//refraction it would not make sense to get a value smaller than 1. This is obtained by
//fitting, not the y[i] values, but the yn[i] values, where:
//   yn[i] = y[i] - ylow - 1/(y[i] - ylow)
// The reverse transformation is then applied when giving the result.
// If a lower bound is requested and if one of the value y[i] is below the bound, it will
//be modified and set slightly above the bound. A warning will be issued.
// The possibility also exists to set an upper bound. See the arguments upbounded and
//yup. But here the fit is NOT modified and CAN give values above yup. It is only when
//calling method V() that if the fitted value is above yup it is replaced by yup! Be
//aware of the difference between lower and upper bound.
//
//   Arguments:
//
// name        : [no default]    name of this spline fit
// title       : [no default]    title of this spline fit
// cat         : [no default]    category to which this spline fit belongs (arbitrary)
// m           : [no default]    number of measurements per spline. m MUST be >1 !
// h           : [no default]    2D histogram from which to get measurements x,y,v,sig
// lowbounded  : [default false] v value have a lower bound. The fit is not allowed to go
//                                below this bound
// vlow        : [default 0.0]   lower bound for v. [irrelevant if lowbounded false]
// upbounded   : [default false] value have an upper bound. The value returned by V(x,y) is
//                                cut at vup
// vup         : [default  1.0]  upper bound for v. [irrelevant if upbounded false]
// debug       : [default false] if true, print matrix and vector of the problem and quality of solution.
//
  const Double_t zero = 0.0;
  Int_t M,i,j,k,kz;
  Int_t nbinsx,nbinsy;
  Double_t xmin,xmax,ymin,ymax,tmin,tmax;
  TAxis *xaxis,*yaxis;
  Bool_t ok;
  Int_t ifail;
  Init(); FindDate();
  ok = InitCatAndBounds(cat,lowbounded,vlow,upbounded,vup);
  VerifyNT();
  xaxis   = h->GetXaxis();
  yaxis   = h->GetYaxis();
  nbinsx  = xaxis->GetNbins();
  xmin    = xaxis->GetXmin();
  xmax    = xaxis->GetXmax();
  nbinsy  = yaxis->GetNbins();
  ymin    = yaxis->GetXmin();
  ymax    = yaxis->GetXmax();
  fZigZag = new TZigZag(nbinsx,xmin,xmax,nbinsy,ymin,ymax);
  M       = nbinsx*nbinsy;
  if (m<=1) {
    m=2;
    std::cout << "2D constructor of TSplineFit: nb of measurements per spline" << std::endl;
    std::cout << "  must be >1 ! We set it to 2" << std::endl;
  }
  fType = SplineFit2D;
  fProvidedH2D = h;
  InitDimensions(M,m);
  tmin = zero;
  tmax = fZigZag->TMax();
  for (j=1;j<=nbinsy;j++) {
    for (i=1;i<=nbinsx;i++) {
      k = h->GetBin(i,j);
      kz = fZigZag->NToZZ(i,j);
      fMt[kz] = fZigZag->T(kz);
      fMv[kz] = h->GetBinContent(k);
      fMs[kz] = h->GetBinError(k);
    }
  }
  ok = InitCheckBounds();
  InitIntervals(tmin,tmax);
  Fill();
  ifail = Solve(debug);
  if (ifail) {
    std::cout << "TSplineFit::TSplineFit Problem in finding solution" << std::endl;
    std::cout << "   ifail = " << ifail << std::endl;
  }
  else  {
    AddThisFit();
    if (gSplineFit) gSplineFit->CutLinkWithHisto();
    gSplineFit = this;
  }
  fA.ResizeTo(1,1);
  fB.ResizeTo(1,1);
}
TSplineFit::TSplineFit(Text_t *name,Text_t *title,Int_t cat,Int_t m,
  TH3D *h,Bool_t lowbounded,Double_t vlow,Bool_t upbounded,
  Double_t vup,Bool_t debug):TNamed(name,title) {
//
// 8TH constructor. 3D Spline fit. m must be >1. No possibility of Spline interpolation.
//                  Measurements provided in an histogram of type TH3D
//
// Constructor taking the measurements from a 3D histogram. All channels of the histogram
//are taken for the fit. Notice the restriction that:
//  - the points must be equidistant in x,y and z, i.e. the 1st constructor of TH3D must have
//      been used:
//      TH3D(const char* name, const char* title, Int_t nbinsx, Axis_t xlow, Axis_t xup,
//           Int_t nbinsy, Axis_t ylow, Axis_t yup,Int_t nbinsz, Axis_t zlow, Axis_t zup)
// The trick explained in the class description of TZigZag is used in order to get a one
//dimensionnal structure from the 3 dimensionnal structure of the histogram. A Spline
//fit in one dimension is then possible. When later on the value of the fit for the point
//(x,y,z) is asked (by calling V(x,y,z), the 8 points nearest to (x,y,z) are found by the method
//TZigZag::NearestPoints. They are delivered with weights. The values of the fit at these 8
//points is then calculated, and the value returned by V(x,y,z) is the weighted mean of
//these 8 values.
//
// TSplineFit has name and title. It is because TSplineFit is intended to be used for instance
//for representing variable parameters inside a Monte-Carlo. In that case, all the variable
//parameters will be stored into the collection fgFits. We want the variable parameters to be
//retrievable by their name and it is why we assign a name and a title to TSplineFit.
// These variable parameters may also belong to different categories, it is also why we include
//an arbitrary category "cat" among the parameters. To increase the speed of the search in fgFits
//when the category of the searched TSplineFit is known, we have at our disposal the static
//TArrayI fgCat, fgCat[i] being the position in fgFits of the first element of category i.
//The categories must be less than 99.
// For instance, in a Monte-Carlo like Litrani, variable parameters may belong to the following
//categories:
//   - index of refraction of materials                    (category 1)
//   - Element of dielectric tensor                        (category 2)
//   - Real part of index of refraction of revetments      (category 3)
//   - Imaginary part of index of refraction of revetments (category 4)
//   - ..............................................
//   - Gain profile of an APD                              (category 13)
//   - and so on...
// We impose also the following restriction concerning name and title:
//if you are entering the first element of the category i,
//   - the name must have 2 parts, separated by '_':
//       -- Before '_', give a name for the category
//       -- After  '_', give a name for the fit
//   - the title must have 2 parts, separated by " | ":
//       -- Before " | ", give a more detailed explanation for the category
//       -- After  " | ", give a more detailed explanation for the fit
//if you are entering following elements of the same category i,
//       --it is no more necessary to give the part preceeding '_' in the name, it will be
//          prepended.
//       --it is no more necessary to give the part preceeding " | " in the title, it will be
//          prepended.
// As an example, suppose your program needs the imaginary part of the index of refraction
//of aluminium and of mylar. You could decide to give category number 3 for imaginary
//part of index of refraction of wrappings and call the 2 TSplineFit like this:
//
//  TSplineFit *imalu   = new TSplineFit("ImIndex_Aluminium",
//              "Imaginary part of refraction index | Standard aluminium",3,...);
//  TSplineFit *immylar = new TSplineFit("Mylar","mylar from 3M",3,...);
//
// The possibility exists to impose a lower bound ylow for the fitted value y. Look at the
//arguments lowbounded and ylow. This is interesting for instance in case you fit a value
//for which a negative value does not make sense. Or for instance when fitting an index of
//refraction it would not make sense to get a value smaller than 1. This is obtained by
//fitting, not the y[i] values, but the yn[i] values, where:
//   yn[i] = y[i] - ylow - 1/(y[i] - ylow)
// The reverse transformation is then applied when giving the result.
// If a lower bound is requested and if one of the value y[i] is below the bound, it will
//be modified and set slightly above the bound. A warning will be issued.
// The possibility also exists to set an upper bound. See the arguments upbounded and
//yup. But here the fit is NOT modified and CAN give values above yup. It is only when
//calling method V() that if the fitted value is above yup it is replaced by yup! Be
//aware of the difference between lower and upper bound.
//
//   Arguments:
//
// name        : [no default]    name of this spline fit
// title       : [no default]    title of this spline fit
// cat         : [no default]    category to which this spline fit belongs (arbitrary)
// m           : [no default]    number of measurements per spline. m MUST be >1 !
// h           : [no default]    3D histogram from which to get measurements x,y,z,v,sig
// lowbounded  : [default false] v value have a lower bound. The fit is not allowed to go
//                                below this bound
// vlow        : [default 0.0]   lower bound for v. [irrelevant if lowbounded false]
// upbounded   : [default false] value have an upper bound. The value returned by V(x,y) is
//                                cut at vup
// vup         : [default  1.0]  upper bound for v. [irrelevant if upbounded false]
// debug       : [default false] if true, print matrix and vector of the problem and quality of solution.
//
  const Double_t zero = 0.0;
  Int_t M,i,j,q,k,kz;
  Int_t nbinsx,nbinsy,nbinsz;
  Double_t xmin,xmax,ymin,ymax,zmin,zmax,tmin,tmax;
  TAxis *xaxis,*yaxis,*zaxis;
  Bool_t ok;
  Int_t ifail;
  Init(); FindDate();
  ok = InitCatAndBounds(cat,lowbounded,vlow,upbounded,vup);
  VerifyNT();
  xaxis   = h->GetXaxis();
  yaxis   = h->GetYaxis();
  zaxis   = h->GetZaxis();
  nbinsx  = xaxis->GetNbins();
  xmin    = xaxis->GetXmin();
  xmax    = xaxis->GetXmax();
  nbinsy  = yaxis->GetNbins();
  ymin    = yaxis->GetXmin();
  ymax    = yaxis->GetXmax();
  nbinsz  = zaxis->GetNbins();
  zmin    = zaxis->GetXmin();
  zmax    = zaxis->GetXmax();
  fZigZag = new TZigZag(nbinsx,xmin,xmax,nbinsy,ymin,ymax,nbinsz,zmin,zmax);
  M       = nbinsx*nbinsy*nbinsz;
  if (m<=1) {
    m=2;
    std::cout << "3D constructor of TSplineFit: nb of measurements per spline" << std::endl;
    std::cout << "  must be >1 ! We set it to 2" << std::endl;
  }
  fType = SplineFit3D;
  fProvidedH3D = h;
  InitDimensions(M,m);
  tmin = zero;
  tmax = fZigZag->TMax();
  for (q=1;q<=nbinsz;q++) {
    for (j=1;j<=nbinsy;j++) {
      for (i=1;i<=nbinsx;i++) {
        k = h->GetBin(i,j,q);
        kz = fZigZag->NToZZ(i,j,q);
        fMt[kz] = fZigZag->T(kz);
        fMv[kz] = h->GetBinContent(k);
        fMs[kz] = h->GetBinError(k);
      }
    }
  }
  ok = InitCheckBounds();
  InitIntervals(tmin,tmax);
  Fill();
  ifail = Solve(debug);
  if (ifail) {
    std::cout << "TSplineFit::TSplineFit Problem in finding solution" << std::endl;
    std::cout << "   ifail = " << ifail << std::endl;
  }
  else  {
    AddThisFit();
    if (gSplineFit) gSplineFit->CutLinkWithHisto();
    gSplineFit = this;
  }
  fA.ResizeTo(1,1);
  fB.ResizeTo(1,1);
}
TSplineFit::TSplineFit(const TSplineFit &fit) {
//
// 9TH constructor. copy constructor
//
  Int_t nr;
  Int_t nrow,ncol,rowlwb,rowupb,collwb,colupb;
  SetName(fit.GetName());
  SetTitle(fit.GetTitle());
  Init();
  fType          = fit.fType;
  fCat           = fit.fCat;
  fNbInFamily    = fit.fNbInFamily;
  fM             = fit.fM;
  fMi            = fit.fMi;
  fMl            = fit.fMl;
  fN             = fit.fN;
  fNs2           = fit.fNs2;
  fSlope         = fit.fSlope;
  fCst           = fit.fCst;
  fBoundedLow    = fit.fBoundedLow;
  fLowBound      = fit.fLowBound;
  fBoundedUp     = fit.fBoundedUp;
  fUpBound       = fit.fUpBound;
//
  nrow           = fit.fA.GetNrows();
  ncol           = fit.fA.GetNcols();
  if ((nrow>0) && (ncol>0)) {
    rowlwb         = fit.fA.GetRowLwb();
    rowupb         = fit.fA.GetRowUpb();
    collwb         = fit.fA.GetColLwb();
    colupb         = fit.fA.GetColUpb();
    fA.ResizeTo(rowlwb,rowupb,collwb,colupb);
    fA             = fit.fA;
  }
  else fA.Clear();
  nrow           = fit.fB.GetNrows();
  ncol           = fit.fB.GetNcols();
  if ((nrow>0) && (ncol>0)) {
    rowlwb         = fit.fB.GetRowLwb();
    rowupb         = fit.fB.GetRowUpb();
    collwb         = fit.fB.GetColLwb();
    colupb         = fit.fB.GetColUpb();
    fB.ResizeTo(rowlwb,rowupb,collwb,colupb);
    fB             = fit.fB;
  }
  else fB.Clear();
  nr             = fit.fX.fN;
  if (nr>0) {
    fX.Set(nr);
    fX           = fit.fX;
  }
  else      fX.Reset();
  fKhi           = fit.fKhi;
  fMt            = fit.fMt;
  fMv            = fit.fMv;
  fMs            = fit.fMs;
  fUseForRandom  = fit.fUseForRandom;
// we do not copy the fUseForRandom property. If you want it, please restore it
//by a call to UseForRandom().
  fParameter     = fit.fParameter;
  fParameterDef  = fit.fParameterDef;
  if (fit.fZigZag) fZigZag = new TZigZag(*fit.fZigZag);
  else              fZigZag = 0;
  if (fit.fInterpolation) Interpolation();
  fDate          = fit.fDate;
  fSource        = fit.fSource;
  fMacro         = fit.fMacro;
  fXLabel        = fit.fXLabel;
  fYLabel        = fit.fYLabel;
  fZLabel        = fit.fZLabel;
  fVLabel        = fit.fVLabel;
//
  fMemoryReduced = fit.fMemoryReduced;
//the function with the provided histo will not be transmitted. Cleaned by Init().
//  ==> fProvidedName  = "";
//  ==> fSplineFitFunc = 0;
//the graphs for plotting will be reserved when plots asked for. Cleaned by Init();
//  ==> fPointsGraph   = 0;
//  ==> fSplineGraph   = 0;
//  ==> fPS            = 0;
//histos not restored. Cleaned by Init()
//  ==> f2Drestored    = kFALSE;
//  ==> f3Drestored    = kFALSE;
//  ==> fProvidedH2D   = 0;
//  ==> fProvidedH3D   = 0;
  fProvidedH1D   = fit.fProvidedH1D;
//histos to show 2D or 3D fits will be reserved when plots asked for. Cleaned by Init();
//  ==> fH2Dfit        = 0;
//  ==> fH3Dfit        = 0;
}
TSplineFit::~TSplineFit() {
// Destructor. If fgFitFile is != 0, then the destructor is called by ROOT. In that
//case, we do not want ROOT to try to modify the collection. Else we also remove
//the fit from the collection.
  fgCounter--;
  CutLinkWithHisto();
  ClearGraphs();
  if (fZigZag)        delete fZigZag;
  if (fInterpolation) delete fInterpolation;
  if (gSplineFit==this) gSplineFit = 0;
  if (!fgFitFile) {
    if (IsInCollection()) {
      fgFits->Remove(this);
      AdjustfgCat();
    }
  }
}
void TSplineFit::AddFit(TSplineFit *pf) {
// Add fit pf to the collection of fits. Do not add it into the data base file.
//For that call UpdateFile().
    TSplineFit *pfit;
    if (!fgFits) fgFits = new TObjArray();
    TIter next(fgFits);
//Look first if fit already there
    Bool_t already = kFALSE;
    while ((!already) && (pfit = (TSplineFit *)next())) {
      if (pfit==pf) {
        already = kTRUE;
        fgFits->Remove(pfit);
        delete pfit;
      }//end if (IsEqual(pfit))
    }//end while ((!already) && (pfit = (TSplineFit *)next()))
    fgFits->Add(pf);
    AdjustfgCat();
}
void TSplineFit::AddNumbering(Int_t k,TString &s) {
//Add the number k in the form of 3 digits, preceded by "__" to the string s
  if ((k>=0) && (k<1000)) {
    s.Append("__");
    if (k<10) s.Append("00");
    else if (k<100) s.Append('0');
    s += k;
  }
  else std::cout << "TSplineFit::AddNumbering : Error : number outside bounds  k = " << k << std::endl;
}
void TSplineFit::AddThisFit() {
//Add this fit to the list of fits. Do not add it into the data base file.
//For that call UpdateFile().
    TSplineFit *pfit;
    TIter next(fgFits);
//Look first if fit already there
    Bool_t already = kFALSE;
    while ((!already) && (pfit = (TSplineFit *)next())) {
      if (IsEqual(pfit)) {
        already = kTRUE;
        fgFits->Remove(pfit);
        delete pfit;
      }//end if (IsEqual(pfit))
    }//end while ((!already) && (pfit = (TSplineFit *)next()))
    fgFits->Add(this);
    AdjustfgCat();
}
void TSplineFit::AdjustErrors() {
// In case y is transformed because od fBoundedLow, the errors must also be transformed
  const Double_t un = 1.0;
  Int_t i;
  Double_t y1,y2,yn1,yn2,t1,t2;
  for (i=0;i<fM;i++) {
    y1     = fMv[i];
    y2     = y1 + fMs[i];
    t1     = y1 - fLowBound;
    yn1    = t1 - un/t1;
    t2     = y2 - fLowBound;
    yn2    = t2 - un/t2;
    fMs[i] = TMath::Abs(yn2-yn1);
  }
}
void TSplineFit::AdjustfgCat() {
//fgCat must point towards the first element of each category. We sort fgFits first.
  const Int_t N = 100;
  Int_t k;
  Int_t i,n;
  TSplineFit *pfit;
  if (!fgCat) fgCat = new TArrayI(100);
  TIter next(fgFits);
  fgFits->UnSort();
  fgFits->Sort();
  for (i=0;i<N;i++) (*fgCat)[i] = -1;
  n = fgFits->GetEntries();
  if (n>0) {
    i = 0;
    while ((pfit = (TSplineFit *)next())) {
      k = pfit->fCat;
      if ((*fgCat)[k] == -1) (*fgCat)[k] = i;
      i++;
    }
  }
}
Double_t TSplineFit::Alpha(Int_t k,Int_t m,Int_t n) const {
// Calculates 4*4 matrices inside matrix fA: this is the part depending upon the measured
//values.
  const Double_t zero = 0.0;
  Int_t i,j,npt,mpn;
  Double_t f,x,xmn,s;
  Double_t a = zero;
  if (k==fN-1) npt = fMl;
  else         npt = fMi;
  for (i=0;i<npt;i++) {
    j    = fMi*k + i;
    x    = fSlope*fMt[j] + fCst;
    s    = fMs[j];
    mpn  = m + n;
    xmn  = XpowerM(x,mpn);
    f    = (2*xmn)/(s*s);
    a   += f;
  }
  return a;
}
Bool_t TSplineFit::AlreadySeen(TString &sprefixn,TString &sprefixt) const {
// Look whether category with prefix sprefixn and sprefixt has already been introduced
//either in collection or in file. In case of fCat==0, we do not verify in file.
//We reserve category 0 to be fits which are NOT intended to be put in the database
//file.
  Int_t N,i,nb;
  TSplineFit *fit;
  Bool_t ok1,ok2;
  Bool_t already = kFALSE;
  N = fgFits->GetEntries();
  if (N>0) {
    TIter next(fgFits);
    while ((!already) && (fit = (TSplineFit *)next())) {
      if (fit->fCat == fCat) {
        sprefixn = fit->GetName();
        sprefixt = fit->GetTitle();
        ok1 = ExtractPrefixN(sprefixn);
        ok2 = ExtractPrefixT(sprefixt);
        already = (ok1 && ok2);
      }//end if (fit->fCat == fCat)
    }//end while ((!already) && (fit = (TSplineFit *)next()))
  }//end if (N>0)
  if ((!already) && (fCat)) {
    fgFitFile = 0;
    fgFitFile = new TFile(fgFileName->Data(),"READ");
    ok1 = fgFitFile->IsOpen();
    if (ok1) {
      fgFitTree = (TTree *)fgFitFile->Get("AllFits");
      if (fgFitTree) {
        fit = new TSplineFit();
        fgFitBranch = fgFitTree->GetBranch("Fits");
        fgFitBranch->SetAddress(&fit);
        N = (Int_t)fgFitTree->GetEntries();
        nb   = 0;
        i    = 0;
        while ((!already) && (i<N)) {
          nb += fgFitTree->GetEntry(i);
//          std::cout << "TSplineFit::AlreadySeen read tree   i = " << i << std::endl;
          if (fit->fCat == fCat) {
            sprefixn = fit->GetName();
            sprefixt = fit->GetTitle();
            ok1 = ExtractPrefixN(sprefixn);
            ok2 = ExtractPrefixT(sprefixt);
            already = (ok1 && ok2);
          }
          i++;
        }//end while ((!found) && (i<N))
//        std::cout << "TSplineFit::AlreadySeen before delete fit" << std::endl;
        delete fit;
//        std::cout << "TSplineFit::AlreadySeen after  delete fit" << std::endl;
        fit = 0;
      }//end if (fgFitTree)
    }//end if (ok1)
    fgFitFile->Close();
    delete fgFitFile;
    fgFitFile   = 0;
    fgFitTree   = 0;
    fgFitBranch = 0;
  }//end if (!already)
  return already;
}
void TSplineFit::ApplyLowBound(TArrayD &A) const {
// Calculates y values if low bound
  const Double_t un = 1.0;
  Int_t i;
  Double_t t;
  for (i=0;i<fM;i++) {
    t = fMv[i] - fLowBound;
    A[i] = t - un/t;
  }
}
void TSplineFit::BelongsToFamily(Int_t k,Double_t par,Text_t *def) {
// The possibility exists for a fit to belong to a family of fits. Let us consider an
//example:
// - Suppose you have at hand measurements of the index of refraction as a function of
//wavelength of various glasses, differing only by the amount of lead inside the glass.
//You have 4 samples
//         - the 1st one with 1% of lead  in the glass
//         - the 2nd one with 2% of lead  in the glass
//         - the 3rd one with 5% of lead  in the glass
//         - the 4th one with 10% of lead in the glass
// It is then natural to group the 4 measurements and fit into a family. It will also
//offer advantages, in particular the possibility of interpolation between the 4 fits
//(allowing for instance a prediction for the results for a glass containing 7% of lead).
//Look at methods:
//    - FindFitInFamily(Int_t)
//    - FindNextInFamily()
//    - V(Double_t,Double_t,TSplineFit*)
//
// For grouping fits into a family, proceed like this:
//   - give the same name to all fits of the family (digits will be appended automatically
//      to the names of all fits of the family in order not to have different fits with
//      the same name)
//   - for each fit of the family, call BelongsToFamily, with 1st argument k from
//       0 to n-1, where n is the number of elements in the family, 4 in our example.
//   - the second argument of BelongsToFamily is some parameter which changes going
//       from one element of the family to the others. In our example, the percentage
//       of lead. THIS PARAMETER MUST INCREASE WITH THE FIRST ARGUMENT! Always define
//       members of a family in increasing order of their parameter!
//   - the third argument of BelonsToFamily is some text explaining what the second
//      argument is.
//
// CINT code of this example, including trying to get an indication of what would be
//the index of refraction of a glass containing 7% of lead at 500nm:
//
// Double_t index7_500;
// TSplineFit *ind0, *ind1, *ind2, *ind3;
// ind0 = new TSplineFit("Index_leadglass","Index of refraction | glass with 1%  lead",3,...);
// ind0->BelongsToFamily(0,1.0,"percentage of lead");
// ind1 = new TSplineFit("Index_leadglass","Index of refraction | glass with 2%  lead",3,...);
// ind1->BelongsToFamily(1,2.0,"percentage of lead");
// ind2 = new TSplineFit("Index_leadglass","Index of refraction | glass with 5%  lead",3,...);
// ind2->BelongsToFamily(2,5.0,"percentage of lead");
// ind3 = new TSplineFit("Index_leadglass","Index of refraction | glass with 10% lead",3,...);
// ind3->BelongsToFamily(3,10.0,"percentage of lead");
// index7_500 = ind2->V(500.0,7.0,ind3);
//
// After these calls to BelongsToFamily are done, the names of the 4 fits will be:
//"Index_leadglass__000", "Index_leadglass__001", "Index_leadglass__002",
//"Index_leadglass__003". Same digits will also have been appended to the titles.
//
  TString s;
  TSplineFit *previousfit;
  fNbInFamily   = k;
  fParameter    = par;
  fParameterDef = def;
  s = GetName();
  AddNumbering(k,s);
  SetName(s.Data());
  s = GetTitle();
  AddNumbering(k,s);
  SetTitle(s.Data());
  AdjustfgCat();
  if (fNbInFamily>0) {
    previousfit = (TSplineFit *)fgFits->Before(this);
    if (previousfit) {
      if (fParameter<previousfit->fParameter)
        std::cout << "TSplineFit::BelongsToFamily ERROR parameter not in increasing order" << std::endl;
    }
  }
}
Double_t TSplineFit::Beta(Int_t k,Int_t m) const {
// Calculates part of right-hand side, depending upon the measured values
  const Double_t zero = 0.0;
  const Double_t un   = 1.0;
  Int_t i,j,npt;
  Double_t f,x,y,xm,s,t;
  Double_t a = zero;
  if (k==fN-1) npt = fMl;
  else         npt = fMi;
  for (i=0;i<npt;i++) {
    j    = fMi*k + i;
    x    = fSlope*fMt[j] + fCst;
    if (fBoundedLow) {
      t = fMv[j] - fLowBound;
      y = t - un/t;
    }
    else y    = fMv[j];
    s    = fMs[j];
    xm   = XpowerM(x,m);
    f    = (2*xm*y)/(s*s);
    a   += f;
  }
  return a;
}
Bool_t TSplineFit::CheckHistErrors(TH1 *h,Bool_t repair) {
// Check that histogram h has no bin error set to 0 (in fact <0.01). If no bin with 0 error
//found, return true and does nothing else. If bins with 0 error found, then
// - if repair false [default] does nothing else but returning false.
// - if repair true            recalculate the errors calling MultinomialAsWeight() and
//                             returns false.
  const Double_t eps = 0.01;
  Double_t E;
  Bool_t ok = kTRUE;
  Int_t M,N,i;
  M = h->GetNbinsX();
  N = (Int_t)h->GetEntries();
  i = 1;
  while (ok && (i<=N)) {
    E = h->GetBinError(i);
    if (E<eps) ok = kFALSE;
    i++;
  }
  if (repair && (!ok)) MultinomialAsWeight(h);
  return ok;
}
Double_t TSplineFit::Chi2(Bool_t raw) const {
// Calculates the chi2 of this spline fit. Notice that if raw [default kFALSE] is
//
// - true  : it is a "raw" chi2. The sum of the differences in y squared divided by the errors
//            squared is divided by the number of measurements. This gives an idea of the
//            quality of the fit, independent of the type of the fit.
// - false : it is conform to the definition of a chi2. The sum of the differences in y squared
//            divided by the errors squared is divided by the number of measurements less the
//            number of free fit parameters (the number of free fit parameters is the number of
//            parameters less the number of conditions). It gives an idea of the quality of the
//            fit, taking into account the number of parameters the fit is using.
//
  Double_t chi2 = 0.0;
  switch (fType) {
    case NotDefined:
      std::cout << "TSplineFit::Chi2 ERROR: fit is not defined" << std::endl;
      break;
    case LinInterpol:
    case SplineInterpol:
      break;
    case SplineFit1D:
    case SplineFit2D:
    case SplineFit3D:
      Int_t i;
      Double_t x,yfit,num,den;
      Int_t freep,kden;
      for (i=0;i<fM;i++) {
        x     = fMt[i];
        yfit  = V(x);
        num   = (fMv[i] - yfit);
        num  *= num;
        den   = fMs[i];
        den  *= den;
        chi2 += num/den;
      }
      if (raw) chi2 /= fM;
      else {
        freep = fN + 3;
        kden  = fM - freep;
        if (kden==0) kden = 1;
        den   = Double_t(kden);
        chi2 /= den;
      }
      break;
  }
  return chi2;
}
void TSplineFit::ClearGraphs(Bool_t AlsoRandom) {
//Delete all graphs and histograms, if we are owner of them.
  if (fPS)          delete fPS;
  fPointsGraph = 0;
  fSplineGraph = 0;
  fPS          = 0;
  if (AlsoRandom) {
    if (fHGenRandom) {
      delete fHGenRandom;
      fHGenRandom = 0;
    }
    if (fHShowRandom) {
      delete fHShowRandom;
      fHShowRandom = 0;
    }
  }
  if ((f2Drestored) && (fProvidedH2D)) {
    delete fProvidedH2D;
    fProvidedH2D = 0;
  }
  if ((f3Drestored) && (fProvidedH3D)) {
    delete fProvidedH3D;
    fProvidedH3D = 0;
  }
  if (fH2Dfit) {
    delete fH2Dfit;
    fH2Dfit = 0;
  }
  if (fH3Dfit) {
    delete fH3Dfit;
    fH3Dfit = 0;
  }
}
Int_t TSplineFit::Compare(const TObject *obj) const {
// Give an order to TSplineFit to make fgFits->Sort() working
  Int_t pos,cat1,cat2;
  TString s1,s2;
  cat1 = fCat;
  cat2 = ((TSplineFit *)obj)->fCat;
  if (cat1<cat2) pos = -1;
  else {
    if (cat1>cat2) pos = 1;
    else {
      s1   = GetName();
      s2   = ((TSplineFit *)obj)->GetName();
      pos  = s1.CompareTo(s2);
    }
  }
  return pos;
}
void TSplineFit::CutLinkWithHisto() {
// Withdraw associated function fSplineFitFunc from histo fProvidedH1D and forget
//about this histogram
  if (fProvidedH1D) {
    TH1D *h = 0;
// Checks that the user has not deleted the histogram!
    h = (TH1D *)gROOT->FindObject(fProvidedName.Data());
    if (h==fProvidedH1D) {
      if (fSplineFitFunc) {
        TF1 *f1,*f2;
        TList *list = fProvidedH1D->GetListOfFunctions();
        f1 = (TF1 *)list->FindObject(fSplineFitFunc);
        if (f1==fSplineFitFunc) {
          f2 = (TF1 *)list->Remove(f1);
          delete fSplineFitFunc;
          fSplineFitFunc = 0;
        }//end if (f1==fSplineFitFunc)
      }//end if (fSplineFitFunc)
    }//end if (h)
    fProvidedH1D = 0;
    fProvidedName  = "";
  }//end if (fProvidedH1D)
}
void TSplineFit::DrawData(Option_t *option) {
// In case of 1D fit or interpolation, same as DrawFit: plots data AND fit.
// In case of 2D or 3D fit, same as DrawHisto: plots only data, not fit.
//
  switch (fType) {
    case NotDefined:
      std::cout << "TSplineFit::DrawData WARNING type of fit not defined" << std::endl;
      break;
    case LinInterpol:
    case SplineInterpol:
    case SplineFit1D:
      DrawFit();
      break;
    case SplineFit2D:
    case SplineFit3D:
      DrawHisto(option);
      break;
  }
}
void TSplineFit::DrawFit(Option_t *option,Int_t facx, Int_t facy, Int_t facz) {
// For 1D case, draws superimposed graph of the measured points and of the fit.
// For 2D or 3D cases, show only the fit, not the data. To see the data, call
//DrawGata() instead.
// The drawing is inside the canvas booked by gOneDisplay.  gOneDisplay is a
//global pointer towards TOnePadDisplay at your disposal from your CINT code.
//It is a good idea to book yourself gOneDisplay at the beginning of your CINT
//code. Doing so, you will be able to specify yourself all the (numerous)
//characteristics of the canvas and of its pad. All variables of TOnePadDisplay
//are public. Not doing so, you will be obliged to accept the default values
//[which are not so bad!]. You can also book gOneDisplay by a call to
//TSplineFit::SetDefaultLabels(). TSplineFit::SetDefaultLabels() will replace
//the default labels of TOnePadDisplay (not specific to this fit) by the default
//labels specific to this fit. For the other options, you will have the defaults
//of TOnePadDisplay.
// If you choose to book gOneDisplay yourself at the beginning of your CINT
//code, do it like this ("blabla1", "blabla2" and "blabla3" will appear inside
//the 3 labels used by the canvas.
//
// - if (!gOneDisplay) {
//     //[To have a small canvas, change kFALSE into kTRUE]
// -   gOneDisplay = new TOnePadDisplay(TSplineFit::fgProgName.Data(),
// -     TSplineFit::fgWebAddress.Data(),"blabla1","blabla2","blabla3",kFALSE);
// -     //[put here all changes you want to the appearance of the canvas]
// -   gOneDisplay->BookCanvas();
// - }
// - else gOneDisplay->NewLabels("blabla1","blabla2","blabla3");
//
// The 1st argument "option", is the drawing option for 2D or 3d fits. Default "LEGO2"
// The arguments facx, facy and facz concerns also only 2D or 3D fits. If
//
//   nbinx is the number of bins in x of the histogram of the data to be fitted
//   nbiny is the number of bins in y of the histogram of the data to be fitted
//   nbinz is the number of bins in z of the histogram of the data to be fitted
//
// then
//  - the number of bins in x of the histogram showing the fit is nbinx*facx
//  - the number of bins in y of the histogram showing the fit is nbiny*facy
//  - the number of bins in z of the histogram showing the fit is nbinz*facz
//
// defaults : 1,1,1
//
  TAxis *xaxis, *yaxis, *zaxis;
  if (!gOneDisplay) SetDefaultLabels();
  else gOneDisplay->fPad->cd();
  switch (fType) {
    case NotDefined:
      std::cout << "TSplineFit::DrawFit WARNING type of fit not defined" << std::endl;
      break;
    case LinInterpol:
    case SplineInterpol:
    case SplineFit1D:
      if (!fPS) FillGraphs();
//      gPad->Clear();
      fPS->Draw("APsame");
      if (fXLabel.Length()>0) {
        xaxis = fPS->GetXaxis();
        xaxis->SetTitle(fXLabel.Data());
      }
      if (fVLabel.Length()>0) {
        yaxis = fPS->GetYaxis();
        yaxis->SetTitle(fVLabel.Data());
      }
      gPad->Modified();
      gPad->Update();
      break;
    case SplineFit2D:
      FillH2D3D(facx,facy);
      gPad->Clear();
      fH2Dfit->Draw(option);
      if (fXLabel.Length()>0) {
        xaxis = fH2Dfit->GetXaxis();
        xaxis->SetTitle(fXLabel.Data());
      }
      if (fYLabel.Length()>0) {
        yaxis = fH2Dfit->GetYaxis();
        yaxis->SetTitle(fYLabel.Data());
      }
      if (fVLabel.Length()>0) {
        zaxis = fH2Dfit->GetZaxis();
        zaxis->SetTitle(fVLabel.Data());
      }
      gPad->Modified();
      gPad->Update();
      break;
    case SplineFit3D:
      FillH2D3D(facx,facy,facz);
      gPad->Clear();
      fH3Dfit->Draw();
      if (fXLabel.Length()>0) {
        xaxis = fH3Dfit->GetXaxis();
        xaxis->SetTitle(fXLabel.Data());
      }
      if (fYLabel.Length()>0) {
        yaxis = fH3Dfit->GetYaxis();
        yaxis->SetTitle(fYLabel.Data());
      }
      if (fZLabel.Length()>0) {
        zaxis = fH3Dfit->GetZaxis();
        zaxis->SetTitle(fZLabel.Data());
      }
      gPad->Modified();
      gPad->Update();
      break;
  }
}
void TSplineFit::DrawFitsInCollection() {
// Shows the drawing of all fits in the collection fgFits. To go from one drawing to
//the next, hit 'n' and then <CR>. To stop the show hit 'q' and then <CR>
  Int_t i,N;
  Char_t c;
  TSplineFit *fit;
  N = fgFits->GetEntries();
  if (N>0) {
    i=0;
    do {
      fit = (TSplineFit *)(*fgFits)[i];
      fit->SetDefaultLabels();
      fit->DrawFit();
      std::cout << "enter n to continue  q to quit : ";
      std::cin  >> c;
      fit->ClearGraphs();
      i++;
    } while ((i<N) && (c!='q'));
  }
  delete gOneDisplay;
  gOneDisplay = 0;
}
void TSplineFit::DrawFitsInFile() {
// Shows the drawing of all fits in the "database" root file fgFitFile. To go from one
//drawing to the next, hit 'n' and then <CR>. To stop the show hit 'q' and then <CR>.
  Int_t i,nb,N;
  Char_t c;
  TSplineFit *fit;
  if (!fgFileName) NameFile();
  fgFitFile = new TFile(fgFileName->Data(),"READ");
  fgFitTree = (TTree *)fgFitFile->Get("AllFits");
  fit = new TSplineFit();
  fgFitBranch = fgFitTree->GetBranch("Fits");
  fgFitBranch->SetAddress(&fit);
  N  = (Int_t)fgFitTree->GetEntries();
  nb = 0;
  if (N>0) {
    i=0;
    do {
      nb += fgFitTree->GetEntry(i);
      fit->SetDefaultLabels();
      fit->DrawFit();
      std::cout << "enter n to continue  q to quit : ";
      std::cin  >> c;
      fit->ClearGraphs();
      i++;
    } while ((i<N) && (c!='q'));
  }
  delete gOneDisplay;
  gOneDisplay = 0;
  delete fit;
  fgFitFile->Close();
  delete fgFitFile;
  fgFitFile   = 0;
  fgFitTree   = 0;
  fgFitBranch = 0;
}
void TSplineFit::DrawHere(Double_t ymin,Double_t ymax) {
// Identical to DrawFit, but do not use TOnePadDisplay!
// For 1D case, draws superimposed graph of the measured points and of the fit.
// For 2D or 3D cases, show only the fit, not the data. To see the data, call
//  DrawGata() instead.
// The 1st argument "option", is the drawing option for 2D or 3d fits. Default "LEGO2"
// The arguments facx, facy and facz concerns also only 2D or 3D fits. If
//
//   nbinx is the number of bins in x of the histogram of the data to be fitted
//   nbiny is the number of bins in y of the histogram of the data to be fitted
//   nbinz is the number of bins in z of the histogram of the data to be fitted
//
// then
//  - the number of bins in x of the histogram showing the fit is nbinx*facx
//  - the number of bins in y of the histogram showing the fit is nbiny*facy
//  - the number of bins in z of the histogram showing the fit is nbinz*facz
//
// defaults : 1,1,1
// If given, ymin and ymax impose range for y axis in 1D plot.
//
  Option_t *option = "LEGO2";
  TAxis *xaxis, *yaxis, *zaxis;
  Int_t    facx   = 1;
  Int_t    facy   = 1;
  Int_t    facz   = 1;
  switch (fType) {
    case NotDefined:
      std::cout << "TSplineFit::DrawFit WARNING type of fit not defined" << std::endl;
      break;
    case LinInterpol:
    case SplineInterpol:
    case SplineFit1D:
      if (!fPS) FillGraphs();
//      gPad->Clear();
      if (ymax>ymin) {
        fPS->SetMinimum(ymin);
        fPS->SetMaximum(ymax);
      }
      fPS->Draw("APsame");
      if (fXLabel.Length()>0) {
        xaxis = fPS->GetXaxis();
        xaxis->SetTitle(fXLabel.Data());
      }
      if (fVLabel.Length()>0) {
        yaxis = fPS->GetYaxis();
        yaxis->SetTitle(fVLabel.Data());
      }
      gPad->Modified();
      gPad->Update();
      break;
    case SplineFit2D:
      FillH2D3D(facx,facy);
      gPad->Clear();
      fH2Dfit->Draw(option);
      if (fXLabel.Length()>0) {
        xaxis = fH2Dfit->GetXaxis();
        xaxis->SetTitle(fXLabel.Data());
      }
      if (fYLabel.Length()>0) {
        yaxis = fH2Dfit->GetYaxis();
        yaxis->SetTitle(fYLabel.Data());
      }
      if (fVLabel.Length()>0) {
        zaxis = fH2Dfit->GetZaxis();
        zaxis->SetTitle(fVLabel.Data());
      }
      gPad->Modified();
      gPad->Update();
      break;
    case SplineFit3D:
      FillH2D3D(facx,facy,facz);
      gPad->Clear();
      fH3Dfit->Draw();
      if (fXLabel.Length()>0) {
        xaxis = fH3Dfit->GetXaxis();
        xaxis->SetTitle(fXLabel.Data());
      }
      if (fYLabel.Length()>0) {
        yaxis = fH3Dfit->GetYaxis();
        yaxis->SetTitle(fYLabel.Data());
      }
      if (fZLabel.Length()>0) {
        zaxis = fH3Dfit->GetZaxis();
        zaxis->SetTitle(fZLabel.Data());
      }
      gPad->Modified();
      gPad->Update();
      break;
  }
}
void TSplineFit::DrawHisto(Option_t *option) {
// Draws the histogram provided. In case of 1D, the SplineFit should appear on the drawing.
//
  TAxis *xaxis, *yaxis, *zaxis;
  if (!gOneDisplay) SetDefaultLabels();
  else gOneDisplay->fPad->cd();
  switch (fType) {
    case NotDefined:
      std::cout << "TSplineFit::DrawHisto WARNING type of fit not defined" << std::endl;
      break;
    case LinInterpol:
    case SplineInterpol:
    case SplineFit1D:
      if (fProvidedH1D) {
        fProvidedH1D->Draw();
        if (fXLabel.Length()>0) {
          xaxis = fProvidedH1D->GetXaxis();
          xaxis->SetTitle(fXLabel.Data());
        }
        if (fVLabel.Length()>0) {
          yaxis = fProvidedH1D->GetYaxis();
          yaxis->SetTitle(fVLabel.Data());
        }
        gPad->Modified();
        gPad->Update();
      }
      else {
        std::cout << "TSplineFit::DrawHisto WARNING no histo to be drawn" << std::endl;
        std::cout << "                      Call DrawFit instead" << std::endl;
      }
     break;
    case SplineFit2D:
      if (!fProvidedH2D) RestoreHisto();
      gPad->Clear();
      fProvidedH2D->Draw(option);
      if (fXLabel.Length()>0) {
        xaxis = fProvidedH2D->GetXaxis();
        xaxis->SetTitle(fXLabel.Data());
      }
      if (fYLabel.Length()>0) {
        yaxis = fProvidedH2D->GetYaxis();
        yaxis->SetTitle(fYLabel.Data());
      }
      if (fVLabel.Length()>0) {
        zaxis = fProvidedH2D->GetZaxis();
        zaxis->SetTitle(fVLabel.Data());
      }
      gPad->Modified();
      gPad->Update();
      break;
    case SplineFit3D:
      if (!fProvidedH3D) RestoreHisto();
      gPad->Clear();
      fProvidedH3D->Draw();
      if (fXLabel.Length()>0) {
        xaxis = fProvidedH3D->GetXaxis();
        xaxis->SetTitle(fXLabel.Data());
      }
      if (fYLabel.Length()>0) {
        yaxis = fProvidedH3D->GetYaxis();
        yaxis->SetTitle(fYLabel.Data());
      }
      if (fZLabel.Length()>0) {
        zaxis = fProvidedH3D->GetZaxis();
        zaxis->SetTitle(fZLabel.Data());
      }
      gPad->Modified();
      gPad->Update();
      break;
  }
}
void TSplineFit::DrawNextInCollection() {
// Draws fit pointed to by fgNextDraw and increment fgNextDraw
  Int_t N;
  TSplineFit *fit;
  if (!fgFits) fgFits = new TObjArray();
  N = fgFits->GetEntries();
  if (N>0) {
    fit = (TSplineFit *)(*fgFits)[fgNextDraw];
    fit->ClearGraphs();
    fit->SetDefaultLabels();
    fit->DrawFit();
    fgNextDraw++;
    if (fgNextDraw>=N) fgNextDraw = 0;
  }
}
void TSplineFit::ErrorsFromFit() {
// This is one of the most interesting method of TSplineFit. The distribution
//of hits inside an histogram containing N hits in all is described by the multinomial
//distribution, epsi being the (unknown) probability for a hit to fall into bin i. The sum
//of all epsi of all bins being 1. The epsi being unknown and the number of hits inside bin i
//being ni, the multinomial distribution gives epsi = ni/N as the most probable value for
//epsi. The error on bin i is then Sqrt(N*epsi*(1-epsi)) = Sqrt(ni*(1-ni/N)).
// The horrible feature of this formula is that it gives an error of 0 (i.e. a infinite
//weight!) for the bins containing 0 hits. It means that in a mathematically strict
//point of view, it is impossible to make a fit on an histogram having some bins with
//0 content. The weight of these bins being infinite, the other bins are negligible and
//the only mathematically correct solution for a fit on the histogram is y==0 for all x!
// The way physicists generally turn around of this problem is to arbitrary set the weight
//of these bins with 0 content to 0, going from an extremely wrong solution to an other
//extremely wrong solution! Indeed the fact of having received 0 hit in some bin is a
//clear indication that the epsi of this bin is small, and so that the weight on this bin,
//however not being infinite, is surely big. Putting it to 0 is totally wrong.
//
// TSplineFit offers a way out of this awful dilemma:
//
//  (1) - call the 3rd or the 4th constructor of TSplineFit giving the histogram for which
//         you want a reasonable (not 0, not infinite) estimation of the errors.
//  (2) - do not forget to ask for a lower bound of 0 (3rd argument of the 4th constructor)
//  (3) - use the result of the spline fit to get an estimation of the epsi for each bin.
//         you can be absolutly sure to never obtain a value of epsi=0, due to the way
//         low bounded fits are implemented in TSplineFit.
//  (4) - recalculate the errors of the fitted bins using the obtained epsi and the
//         formula Sqrt(N*epsi*(1-epsi)). Put these new errors in the histogram using
//         the method TH1::SetBinError().
//  (5) - then you are able to do fits on the histogram which are not meaningless!!
//
// It is exactly what this method does!
//
// Careful readers have certainly detected here a snake biting its tail: we want reasonable
//errors to do a fit and propose the result of the fit to have reasonable errors. How to
//do the first fit (this one, the spline fit) without having yet reasonable errors? For
//the first fit, we make a check of the errors. If we detect bins with 0 error, we replace
//all the errors of all bins by errors calculated using the formula obtained when taking
//the multinomial distribution as a weight function. These errors are never 0, even if the
//content of the bin is 0. This is done by the method MultinomialAsWeight().
// Then we proceed by iteration. The user is then free to repeat the fit as many times as
//he wants, having then better and better errors, and smaller lower limits for the errors.
//The CINT code to do this, making for instance the fit 3 times, is (h being a pointer on
//a TH1D histogram, and eps an absolute small lower bound on the errors, in order to prevent
//problems in fitting):
//
//  const Double_t eps = 1.0e-2;
//  TSplineFit *sf = new TSplineFit(h,0,3,kTRUE);
//  sf->ErrorsFromFit();
//  sf->GetDataFromHist(eps);
//  sf->RedoFit();
//  sf->ErrorsFromFit();
//  sf->GetDataFromHist(eps);
//  sf->RedoFit();
//
  if (fProvidedH1D) {
    const Double_t zero = 0.0;
    const Double_t un   = 1.0;
    Int_t i,Nbin;
    Double_t x,xmin,xmax,N,ni,epsi;
    Stat_t error;
    xmin = fKhi[0];
    xmax = fKhi[fN];
    Nbin = fProvidedH1D->GetNbinsX();
    N    = zero;
    TArrayD C(Nbin+1);
    for (i=1;i<=Nbin;i++) {
      x  = fProvidedH1D->GetBinCenter(i);
      if ((x>=xmin) && (x<=xmax)) {
        ni   = V(x);
        N   += ni;
        C[i] = ni;
      }//end if ((x>=xmin) && (x<=xmax))
      else {
        ni   = fProvidedH1D->GetBinContent(i);
        N   += ni;
        C[i] = ni;
      }//end else if ((x>=xmin) && (x<=xmax))
    }//end for (i=1;i<=Nbin;i++)
    for (i=1;i<=Nbin;i++) {
      ni    = C[i];
      epsi  = ni/N;
      error = TMath::Sqrt(N*epsi*(un-epsi));
      fProvidedH1D->SetBinError(i,error);
    }//end for (i=1;i<=Nbin;i++)
  }//end if (fProvidedH1D)
}
Bool_t TSplineFit::ExtractPrefixN(TString &s) const {
//Returns only the prefix part in s where s is the name of a fit
  Bool_t ok = kFALSE;
  Int_t N;
  if (s.Contains("_")) {
    ok = kTRUE;
    N  = s.Index("_",1);
    s.Resize(N+1);
  }
  return ok;
}
Bool_t TSplineFit::ExtractPrefixT(TString &s) const {
//Returns only the prefix part in s where s is the title of a fit
  Bool_t ok = kFALSE;
  Int_t N;
  if (s.Contains(" | ")) {
    ok = kTRUE;
    N  = s.Index(" | ",3);
    s.Resize(N+3);
  }
  return ok;
}
void TSplineFit::Fill() {
// Fill the matrix fA and vector fB of the problem. fA is a band matrix in compact
//format as described in class TBandedLE. See the class description of TBandedLE and
//of TSplineFit.
  const Double_t zero = 0.0;
  const Double_t un   = 1.0;
  Int_t i,j,k,ij,is,js,nrowsA,ncolsA;
  Int_t mband;
  Double_t a;
  TArrayD savefMs(fM);  //to save fMs in case it has to be modified
  if (fBoundedLow) {
    for (i=0;i<fM;i++) savefMs[i] = fMs[i];
    AdjustErrors();
  }
  mband = 2*fgM + 1;
  TMatrixD M(fgU,mband);
  fA.Zero();
  fB.Zero();
  nrowsA = fA.GetNrows();
  ncolsA = fA.GetNcols();
  for (k=0;k<fN;k++) {
    M.Zero();
    for (i=0;i<fgU;i++) {
      for (j=0;j<mband;j++) {
        ij = i*mband + j;
        switch (ij) {
          case 0:
          case 1:
          case 2:
            break;
          case 3:
            M(i,j) = un;
            break;
          case 4:
          case 5:
            break;
          case 6:
            M(i,j) = Alpha(k,0,0);
            break;
          case 7:
            M(i,j) = Alpha(k,0,1);
            break;
          case 8:
            M(i,j) = Alpha(k,0,2);
            break;
          case 9:
            M(i,j) = Alpha(k,0,3);
            break;
          case 10:
            M(i,j) = -un;
            break;
          case 11:
          case 12:
            break;
          case 13:
          case 14:
            break;
          case 15:
            a      = fSlope*fKhi[k] + fCst;
            M(i,j) = a;
            break;
          case 16:
            M(i,j) = un;
            break;
          case 17:
            break;
          case 18:
            M(i,j) = M(0,7);
            break;
          case 19:
            M(i,j) = Alpha(k,1,1);
            break;
          case 20:
            M(i,j) = Alpha(k,1,2);
            break;
          case 21:
            M(i,j) = Alpha(k,1,3);
            break;
          case 22:
            M(i,j) = -(fSlope*fKhi[k+1] + fCst);
            break;
          case 23:
            M(i,j) = -un;
            break;
          case 24:
          case 25:
          case 26:
            break;
          case 27:
            a = fSlope*fKhi[k] + fCst;
            M(i,j) = a*a;
            break;
          case 28:
            M(i,j) = 2*(fSlope*fKhi[k] + fCst);
            break;
          case 29:
            M(i,j) = un;
            break;
          case 30:
            M(i,j) = M(0,8);
            break;
          case 31:
            M(i,j) = M(1,7);
            break;
          case 32:
            M(i,j) = Alpha(k,2,2);
            break;
          case 33:
            M(i,j) = Alpha(k,2,3);
            break;
          case 34:
            a = fSlope*fKhi[k+1] + fCst;
            M(i,j) = -a*a;
            break;
          case 35:
            M(i,j) = -2*(fSlope*fKhi[k+1] + fCst);
            break;
          case 36:
            M(i,j) = -un;
            break;
          case 37:
          case 38:
            break;
          case 39:
            a = fSlope*fKhi[k] + fCst;
            M(i,j) = a*a*a;
            break;
          case 40:
            a = fSlope*fKhi[k] + fCst;
            M(i,j) = 3*a*a;
            break;
          case 41:
            M(i,j) = 3*(fSlope*fKhi[k] + fCst);
            break;
          case 42:
            M(i,j) = M(0,9);
            break;
          case 43:
            M(i,j) = M(1,8);
            break;
          case 44:
            M(i,j) = M(2,7);
            break;
          case 45:
            M(i,j) = Alpha(k,3,3);
            break;
          case 46:
            a = fSlope*fKhi[k+1] + fCst;
            M(i,j) = -a*a*a;
            break;
          case 47:
            a = fSlope*fKhi[k+1] + fCst;
            M(i,j) = -3*a*a;
            break;
          case 48:
            M(i,j) = -3*(fSlope*fKhi[k+1] + fCst);
            break;
          case 49:
          case 50:
          case 51:
          case 52:
          case 53:
            break;
          case 54:
            M(i,j) = -un;
            break;
          case 55:
            M(i,j) = -(fSlope*fKhi[k+1] + fCst);
            break;
          case 56:
            a = fSlope*fKhi[k+1] + fCst;
            M(i,j) = -a*a;
            break;
          case 57:
            a = fSlope*fKhi[k+1] + fCst;
            M(i,j) = -a*a*a;
            break;
          case 58:
          case 59:
          case 60:
            break;
          case 61:
            M(i,j) = un;
            break;
          case 62:
            M(i,j) = fSlope*fKhi[k+1] + fCst;
            break;
          case 63:
            a = fSlope*fKhi[k+1] + fCst;
            M(i,j) = a*a;
            break;
          case 64:
            a = fSlope*fKhi[k+1] + fCst;
            M(i,j) = a*a*a;
            break;
          case 65:
          case 66:
            break;
          case 67:
            M(i,j) = -un;
            break;
          case 68:
            M(i,j) = -2*(fSlope*fKhi[k+1] + fCst);
            break;
          case 69:
            a = fSlope*fKhi[k+1] + fCst;
            M(i,j) = -3*a*a;
            break;
          case 70:
          case 71:
          case 72:
          case 73:
            break;
          case 74:
            M(i,j) = un;
            break;
          case 75:
            M(i,j) = 2*(fSlope*fKhi[k+1] + fCst);
            break;
          case 76:
            a = fSlope*fKhi[k+1] + fCst;
            M(i,j) = 3*a*a;
            break;
          case 77:
          case 78:
          case 79:
            break;
          case 80:
            M(i,j) = -un;
            break;
          case 81:
            M(i,j) = -3*(fSlope*fKhi[k+1] + fCst);
            break;
          case 82:
          case 83:
          case 84:
          case 85:
          case 86:
            break;
          case 87:
            M(i,j) = un;
            break;
          case 88:
            M(i,j) = 3*(fSlope*fKhi[k+1] + fCst);
            break;
          case 89:
          case 90:
            break;
          default:
            std::cout << "TSplineFit::Fill : error on index ij" << std::endl;
            break;
        }//end switch (ij)
      }//end for (j=0;j<mband;j++)
      switch (i) {
        case 0:
        case 1:
        case 2:
        case 3:
          fB(k*fgU+i,0) = Beta(k,i);
          break;
        case 4:
        case 5:
        case 6:
          is = k*fgU+i;
          if (is<nrowsA) fB(is,0) = zero;
          break;
         default:
            std::cout << "TSplineFit::Fill : error on index i" << std::endl;
          break;
      }//end switch (i)
    }//end for (i=0;i<fgU;i++)
    if (k<(fN-1)) {
      for (i=0;i<fgU;i++) {
        is = i + fgU*k;
        for (j=0;j<mband;j++) {
          if (!k) js = j - fgM + i;
          else    js = j;
          if ((js>=0) && (js<ncolsA)) fA(is,js) = M(i,j);
        }//end for (j=0;j<mband;j++)
      }//end for (i=0;i<fgU;i++)
    }//end if (k<(fN-1))
    else {
      for (i=0;i<fgU-3;i++) {
        is = i + fgU*k;
        ij = ncolsA - i - 1;
        for (j=0;j<ij;j++) {
          js = j;
          if ((js>=0) && (js<ncolsA)) fA(is,js) = M(i,j);
        }//end for (j=0;j<ij;j++)
      }//end for (i=0;i<fgU-3;i++)
    }//end else if (k<(fN-1))
  }//end for (k=0;k<fN;k++)
  if (fBoundedLow) for (i=0;i<fM;i++) fMs[i] = savefMs[i];
}
void TSplineFit::FillGraphs(Int_t npt,Color_t color,Style_t style) {
// Fills the graph with the measured values and the fit. For displaying the fit, npt + 1
//points will be used.
  Int_t i,npt1;
  Double_t step,xcur;
  npt1  = npt + 1;
  TArrayD x(npt1),y(npt1);
  xcur  =  fKhi[0];
  step  = (fKhi[fN] -xcur)/npt;
  for (i=0;i<npt1;i++) {
    x[i]  = xcur;
    y[i]  = V(xcur);
    xcur += step;
  }
  if (fPointsGraph) {
    delete fPointsGraph;
    fPointsGraph = 0;
  }
  if (!fMemoryReduced) {
    fPointsGraph = new TGraphErrors(fM,fMt.fArray,fMv.fArray,0,fMs.fArray);
    fPointsGraph->SetName(GetName());
    fPointsGraph->SetTitle(GetTitle());
    fPointsGraph->SetMarkerColor(color);
    fPointsGraph->SetMarkerStyle(style);
    fPointsGraph->SetFillColor(41);
  }
  if (fSplineGraph) delete fSplineGraph;
  fSplineGraph = new TGraph(npt1,x.fArray,y.fArray);
  fSplineGraph->SetMarkerStyle(20);
  fSplineGraph->SetMarkerSize(0.5);
  fSplineGraph->SetMarkerColor(2);
  fSplineGraph->SetFillColor(41);
  if (fPS) delete fPS;
  fPS = new TMultiGraph(GetName(),GetTitle());
  if (!fMemoryReduced) fPS->Add(fPointsGraph);
  fPS->Add(fSplineGraph);
}
Bool_t TSplineFit::FillH2D3D(Int_t facx, Int_t facy, Int_t facz) {
// Creates the histogram fH2Dfit or fH3Dfit in order to show the result of the
//2D or 3D fit. The number of points in these histograms is multiplied by
// - facx in x dimension,
// - facy in y dimension,
// - facz in z dimension,
//as compared with the histogram containing the data.
  Bool_t ok;
  Int_t nx,ny,nz,i,j,m,k;
  TString sn,st;
  Double_t xmin,xmax,ymin,ymax,zmin,zmax,x,y,z;
  TAxis *xaxis, *yaxis,*zaxis;
  sn = GetName();
  sn.Prepend("Fit");
  st = GetTitle();
  st.Prepend("Fit");
  switch (fType) {
    case SplineFit2D:
      nx   = facx*fZigZag->GetNx();
      xmin = fZigZag->GetXmin();
      xmax = fZigZag->GetXmax();
      ny   = facy*fZigZag->GetNy();
      ymin = fZigZag->GetYmin();
      ymax = fZigZag->GetYmax();
      if (fH2Dfit) delete fH2Dfit;
      fH2Dfit = new TH2D(sn.Data(),st.Data(),nx,xmin,xmax,ny,ymin,ymax);
      xaxis = fH2Dfit->GetXaxis();
      yaxis = fH2Dfit->GetYaxis();
      for (j=1;j<=ny;j++) {
        for (i=1;i<=nx;i++) {
          k = fH2Dfit->GetBin(i,j);
          x = xaxis->GetBinCenter(i);
          y = yaxis->GetBinCenter(j);
          fH2Dfit->SetBinContent(k,V(x,y));
        }
      }
      ok = kTRUE;
      break;
    case SplineFit3D:
      nx   = facx*fZigZag->GetNx();
      xmin = fZigZag->GetXmin();
      xmax = fZigZag->GetXmax();
      ny   = facy*fZigZag->GetNy();
      ymin = fZigZag->GetYmin();
      ymax = fZigZag->GetYmax();
      nz   = facz*fZigZag->GetNz();
      zmin = fZigZag->GetZmin();
      zmax = fZigZag->GetZmax();
      if (fH3Dfit) delete fH3Dfit;
      fH3Dfit = new TH3D(sn.Data(),st.Data(),nx,xmin,xmax,ny,ymin,ymax,nz,zmin,zmax);
      xaxis = fH3Dfit->GetXaxis();
      yaxis = fH3Dfit->GetYaxis();
      zaxis = fH3Dfit->GetZaxis();
      for (m=1;m<=nz;m++) {
        for (j=1;j<=ny;j++) {
          for (i=1;i<=nx;i++) {
            k = fH3Dfit->GetBin(i,j,m);
            x = xaxis->GetBinCenter(i);
            y = yaxis->GetBinCenter(j);
            z = zaxis->GetBinCenter(m);
            fH3Dfit->SetBinContent(k,V(x,y,z));
          }
        }
      }
      ok = kTRUE;
      break;
    default:
      ok = kFALSE;
      break;
  }
  return ok;
}
void TSplineFit::FindDate() {
// Finds the date
  Int_t day,month,year,date;
  TDatime *td;
  td = new TDatime();
  date  = td->GetDate();
  day   = date % 100;
  date /= 100;
  month = date % 100;
  date /= 100;
  year  = date;
  delete td;
  fDate  = "  ";
  fDate += day;
  fDate.Append(" / ");
  fDate += month;
  fDate.Append(" / ");
  fDate += year;
  fDate.Append("  ");
}
TSplineFit *TSplineFit::FindFit(const Text_t *name,Int_t cat,Bool_t storeit) {
// Finds the fit
//   - specified by name (the full name, including the name of the category)
//   - specifying the category or not. If you do not specify the category, not
//      giving it or giving a negative number, the whole collection will be searched.
//   - if you specify the category, only the part of the collection devoted to this
//      category is searched: it is faster!
// If the specified fit is not found in the collection fgFits, then the file fgFitFile
//is searched! If fit found in file, it is put into the collection in case storeit
//true [default], it is not in case storeit false.
// 0 returned if fit found neither in collection, nor in file.
//
  Int_t i,N,nfit,nb;
  TString s1;
  TSplineFit *fit = 0;
  Bool_t found = kFALSE;
  if (!fgFileName) NameFile();
  if (!fgCat) fgCat = new TArrayI(100);
  if (!fgFits) fgFits = new TObjArray();
  N = fgFits->GetEntries();
//Search in the collection
  if (N>0) {
    if ((cat>=0) && (cat<100)) {
      i = (*fgCat)[cat];
      if (i>=0) {
        do {
          fit = (TSplineFit *)(*fgFits)[i];
          s1  = fit->GetName();
          found = !s1.CompareTo(name);
          i++;
        } while ((!found) && (fit->fCat<=cat) && (i<N));
      }
    }//end if ((cat>=0) && (cat<100))
    else {
      i = 0;
      do {
        fit = (TSplineFit *)(*fgFits)[i];
        s1  = fit->GetName();
        found = !s1.CompareTo(name);
        i++;
      } while ((!found) && (i<N));
    }//end else if ((cat>=0) && (cat<100))
  }//end if (N>0)
  if (!found) {
//Search in the file
    fgFitFile = new TFile(fgFileName->Data(),"READ");
    fgFitTree = (TTree *)fgFitFile->Get("AllFits");
    fit = new TSplineFit();
    fgFitBranch = fgFitTree->GetBranch("Fits");
    fgFitBranch->SetAddress(&fit);
    nfit = (Int_t)fgFitTree->GetEntries();
    nb   = 0;
    i    = 0;
    while ((!found) && (i<nfit)) {
      nb += fgFitTree->GetEntry(i);
      s1  = fit->GetName();
      if (!s1.CompareTo(name)) found = kTRUE;
      i++;
    }
    if (found) {
      if (storeit) {
        if (cat>=0) {
          if (cat==fit->fCat) AddFit(fit);
          else found = kFALSE;
        }
        else AddFit(fit);
      }
    }
    else {
      delete fit;
      fit = 0;
    }
    fgFitFile->Close();
    delete fgFitFile;
    fgFitFile   = 0;
    fgFitTree   = 0;
    fgFitBranch = 0;
    if (found) fit->Regenerate();
  }//end if (!found)
  if (!found) fit=0;
  return fit;
}
TSplineFit *TSplineFit::FindFirstInFamily(Int_t &kfirst) {
// In case "this" is member of a family, finds the first element of this family.
//Returns the pointer to this first element, and the position k of this first
//element in the ordered collection.
  TSplineFit *fit = 0;
  kfirst = -1;
  if (!fgFits) fgFits = new TObjArray();
  if (fNbInFamily>=0) {
    TString s;
    Int_t N;
    if (fNbInFamily==0) fit = this;
    else {
      s = GetName();
      N = s.Length();
      s.Resize(N-5);
      AddNumbering(0,s);
      fit = FindFit(s.Data(),fCat);
    }
    if (fit) kfirst = fgFits->BinarySearch(fit);
  }
  else std::cout << "TSplineFit::FindFirstInFamily ERROR this is not member of a family" << std::endl;
  return fit;
}
TSplineFit *TSplineFit::FindFirstInFamily(const Text_t *family,Int_t &kfirst) {
// Finds the first element of a family, giving the name of the family.
//Returns the pointer to this first element, and the position k of this first
//element in the ordered collection.
  TString s;
  TSplineFit *fit = 0;
  kfirst = -1;
  if (!fgFits) fgFits = new TObjArray();
  s      = family;
  AddNumbering(0,s);
  fit = FindFit(s.Data());
  if (fit) kfirst = fgFits->BinarySearch(fit);
  return fit;
}
TSplineFit *TSplineFit::FindFitInFamily(Int_t k,Int_t kfirst,Int_t M) {
// VERY IMPORTANT: before calling this method, all fits of the family MUST be
//in the collection fgFits! To do so, call first the static method LoadFamily().
//By that, you will get M, the number of elements in the family! Then call
//method FindFirstInFamily(). By that you get the position kfirst of the
//first element of the family in the collection fgFits.
// - k is the member searched for. 0<=k<M
// - kfirst is the position inside the ordered collection fgFits of the first
//          element of the family.
// - M is the number of elements in the family
// If "this" belongs to a family of fit, retrieves the member k of this family,
//returning its pointer, in case there is a member k in the family. If not
//found, returns null pointer. If member k of the family has not been found in
//the collection fgFits, but has been found in the file fgFitFile, it is
//inserted into the collection.
// Always return a null pointer if "this" does not belong to a family
  TSplineFit *fit = 0;
  if (!fgFits) fgFits = new TObjArray();
  if ((k>=0) && (k<M)) {
    fit = (TSplineFit *)(*fgFits)[kfirst+k];
    if (fit->fNbInFamily<0) fit = 0;
  }
  return fit;
}
void TSplineFit::GetDataFromHist(Double_t Emin) {
// Pull out the data from the histogram. Emin is the minimum value accepted for an error.
//If the error is smaller, then it is replaced by Emin. In this instance of GetDataFromHist,
//xmin and xmax have already been put into fKhi[0] and fKhi[fN].
  Int_t i,k,npoints;
  Bool_t errorsok;
  Double_t x,E,xmin,xmax;
  xmin = fKhi[0];
  xmax = fKhi[fN];
  errorsok = CheckHistErrors(fProvidedH1D,kTRUE);
  npoints  = fProvidedH1D->GetNbinsX();
  k = 0;
  for (i=1;i<=npoints;i++) {
    if (xmin>=xmax) {
      fMt[k] = fProvidedH1D->GetBinCenter(i);
      fMv[k] = fProvidedH1D->GetBinContent(i);
      E      = fProvidedH1D->GetBinError(i);
      if (E<Emin) E=Emin;
      fMs[k] = E;
      k++;
    }
    else {
      x = fProvidedH1D->GetBinCenter(i);
      if ((x>=xmin) && (x<=xmax)) {
        fMt[k] = x;
        fMv[k] = fProvidedH1D->GetBinContent(i);
        E      = fProvidedH1D->GetBinError(i);
        if (E<Emin) E=Emin;
        fMs[k] = E;
        k++;
      }//end if ((x>=xmin) && (x<=xmax))
    }//end else if (xmin>=xmax)
  }//end for (i=0;i<npoints;i++)
}

//Added by Xianglei Zhu.
//To count the dimension of TH1D correctly.
Int_t TSplineFit::GetDimensionHist(Double_t & xmin,Double_t & xmax,Double_t Emin,TH1D *h) {
// Pull out the data from the histogram. Emin is the minimum value accepted for an error.
//If the error is smaller, then it is replaced by Emin. This instance of GetDataFromFit is
//called by the 3rd or the 4th constructor of TSplineFit, and is not available to the
//user.
  Int_t i,k,npoints;
  Bool_t errorsok;
  Double_t x,E;
  Double_t xmin_save=0, xmax_save=0; //XZHU
  //  errorsok = CheckHistErrors(fProvidedH1D,kTRUE);
  //-- modified by Xin Dong
  errorsok = CheckHistErrors(h,kFALSE);
  //--
  npoints = h->GetNbinsX();
  k = 0;
  for (i=1;i<=npoints;i++) {
    if (xmin>=xmax) {
      E      = h->GetBinError(i);
	if (E<1e-6){
	   continue;
	}
	if(k==0) xmin_save = h->GetBinCenter(i) - h->GetBinWidth(i)/2.0;
	else xmax_save = h->GetBinCenter(i) + h->GetBinWidth(i)/2.0;
      k++;
    }
    else {
      x = h->GetBinCenter(i);
      if ((x>=xmin) && (x<=xmax)) {
        E      = h->GetBinError(i);
        if (E<1e-6){
	     continue;
	  }
	  if(k==0) xmin_save = h->GetBinCenter(i) - h->GetBinWidth(i)/2.0;
	  else xmax_save = h->GetBinCenter(i) + h->GetBinWidth(i)/2.0;
        k++;
      }//end if ((x>=xmin) && (x<=xmax))
    }//end else if (xmin>=xmax)
  }//end for (i=0;i<npoints;i++)
  xmin = xmin_save;
  xmax = xmax_save;

  return k;
}

void TSplineFit::GetDataFromHist(Double_t xmin,Double_t xmax,Double_t Emin,TH1D *h) {
// Pull out the data from the histogram. Emin is the minimum value accepted for an error.
//If the error is smaller, then it is replaced by Emin. This instance of GetDataFromFit is
//called by the 3rd or the 4th constructor of TSplineFit, and is not available to the
//user.
  Int_t i,k,npoints;
  Bool_t errorsok;
  Double_t x,E;
  //  errorsok = CheckHistErrors(fProvidedH1D,kTRUE);
  //-- modified by Xin Dong
  errorsok = CheckHistErrors(h,kFALSE);
  //--
  npoints = h->GetNbinsX();
  k = 0;
  for (i=1;i<=npoints;i++) {
    if (xmin>=xmax) {
      E      = h->GetBinError(i);
      //if (E<Emin) E=Emin;
	if (E<1e-6){
	   continue;
	}
      fMt[k] = h->GetBinCenter(i);
      fMv[k] = h->GetBinContent(i);
	fMs[k] = E;
      k++;
    }
    else {
      x = h->GetBinCenter(i);
      if ((x>=xmin) && (x<=xmax)) {
        E      = h->GetBinError(i);
        //if (E<Emin) E=Emin;
        if (E<1e-6){
	     continue;
	  }
        fMt[k] = x;
        fMv[k] = h->GetBinContent(i);
        fMs[k] = E;
        k++;
      }//end if ((x>=xmin) && (x<=xmax))
    }//end else if (xmin>=xmax)
  }//end for (i=0;i<npoints;i++)
}
Text_t *TSplineFit::GetFamilyName() const {
// If "this" belongs to a family, returns the family name. The family name is the name
//of "this", without the numbering at the end. If "this" does not belong to a family,
//returns the empty string
  Int_t N;
  TString s="";
  if (fNbInFamily>=0) {
    s = GetName();
    N = s.Length();
    s.Resize(N-5);
  }
  return (Text_t *)s.Data();
}
Bool_t TSplineFit::GetLowBound(Double_t &lowbound) const {
// If true returned, then lowbound returned is the lower bound, else lowbound
//untouched
  if (fBoundedLow) lowbound = fLowBound;
  return fBoundedLow;
}
Double_t TSplineFit::GetRandom() const {
// Get a random number according to the fitted distribution. UseForRandom(kTRUE) must
//have been called before using GetRandom().
// Do not work with 2D or 3D fits.
// In case fType==SplineFit1D, the distribution must have a low bound of 0.
//
  const Double_t minus = -1.0;
  Double_t z = minus;
  if (fUseForRandom) {
    Int_t bin;
    z = fHGenRandom->GetRandom();
    bin = fHShowRandom->Fill(z);
  }
  else std::cout << "TSplineFit::GetRandom ERROR UseForRandom not called" << std::endl;
  return z;
}
Double_t TSplineFit::GetRandom(Int_t M,Double_t p) {
// Get a random number according to the fitted distribution using the member of
//the family which has a parameter value just below p. Must be called with the
//member of the family with fNbInFamily==0 (the first element of the family)
//UseForRandom(kTRUE) must have been called for ALL members of the family before
//using GetRandom(p).
// Do not work with 2D or 3D fits.
// In case fType==SplineFit1D, the distribution must have a low bound of 0.
//
// IMPORTANT: this method only works if LoadFamily() has been called! All
//members of the family must be inside the ordered collection fgFits. By calling
//LoadFamily(), you also got the number of elements M in the family, which
//you have to provide to this instance of GetRandom().
//
// Parameters:
//
// - M number of elements in the family
// - p value of the family parameter to select the good family member
//
  const Double_t minus = -1.0;
  Double_t z = minus;
  if (fNbInFamily==0) {
    Int_t kfirst;
    TSplineFit *fitnew, *fitold;
    if (M==1) z = GetRandom();
    else {
      Int_t i      = 0;
      Bool_t found = kFALSE;
      fitnew = this;
      Double_t pnew,pold;
      pnew = fParameter;
      if (pnew>=p) {
        found = kTRUE;
        z = GetRandom();
      }
      else {
        kfirst = fgFits->BinarySearch(fitnew);
        while ((!found) && (i<M)) {
          i++;
          fitold = fitnew;
          fitnew = (TSplineFit *)(*fgFits)[kfirst+i];
          pold = pnew;
          pnew = fitnew->fParameter;
          if (pnew>p) {
            found = kTRUE;
            z = fitold->GetRandom();
          }
          else {
            if (i==(M-1)) {
              found = kTRUE;
              z     = fitnew->GetRandom();
            }//end if (i==(M-1))
          }//end else if (pnew>p)
        }//end while ((!found) && (i<M))
      }//end else if (pnew>=p)
    }//end else if (M==1)
  }//end if (fNbInFamily==0)
  else {
    std::cout << "TSplineFit::GetRandom ERROR call GetRandom(.,.) with the 1st member" << std::endl;
    std::cout << "  of the family!" << std::endl;
  }//end else if (fNbInFamily==0)
  return z;
}
Bool_t TSplineFit::GetSpline(Int_t i,Double_t &a,Double_t &b,Double_t &c,
                             Double_t &d) const {
// Give the coefficients a,b,c,d ( y = a + b*x +c*x*x + d*x*x*x ) of the spline of
//the interval i. Returns true if i is a valid interval. But be aware that
//these coefficients a,b,c,d are for a spline acting on normalized x, -1<=xn<=1, not
//x in the user coordinates, xmin<=x<=xmax. So that if you want to calculate
//the value of the spline at x, you have to do (p pointeur to TSplineFit:
//  xn = p->XNorm(x);
//  y  = a + b*xn + c*xn*xn + d*xn*xn*xn
// Be also aware of the fact that because of the variable transform applied in case
//of a low bound, you also have to apply the reverse transformation on y in case of
//low bound.
//
  Int_t k;
  Bool_t inside = kFALSE;
  if ((i>=0) && (i<fN)) {
    inside = kTRUE;
    k = fgU*i;
    a = fX[k];
    b = fX[k+1];
    c = fX[k+2];
    d = fX[k+3];
  }
  return inside;
}
Bool_t TSplineFit::GetSpline(Double_t x,Double_t &a,Double_t &b,Double_t &c,
                             Double_t &d) const {
// Give the coefficients a,b,c,d ( y = a + b*x +c*x*x + d*x*x*x ) of the spline of
//the interval containing x. Return true if x between xmin and xmax. But be aware that
//these coefficients a,b,c,d are for a spline acting on normalized x, -1<=xn<=1, not
//x in the user coordinates, xmin<=x<=xmax. The 1st argument of GetSpline, x, is in
//user coordinate, not in normalized coordinates, so that if you want to calculate
//the value of the spline at x, you have to do (p pointeur to TSplineFit:
//  xn = p->XNorm(x);
//  y  = a + b*xn + c*xn*xn + d*xn*xn*xn
// Be also aware of the fact that because of the variable transform applied in case
//of a low bound, you also have to apply the reverse transformation on y in case of
//low bound.
//
  Bool_t inside;
  Int_t i,k;
  if ( x<fKhi[0] ) {
    inside = kFALSE;
    i=0;
  }
  else {
    if ( x >fKhi[fN] )  {
      inside = kFALSE;
      i = fN - 1;
    }
    else {
      inside = kTRUE;
      i = Interval(x);
    }
  }
  k = fgU*i;
  a = fX[k];
  b = fX[k+1];
  c = fX[k+2];
  d = fX[k+3];
  return inside;
}
Bool_t TSplineFit::GetSplineNN(Int_t i,Double_t &a,Double_t &b,Double_t &c,
                             Double_t &d) const {
// Give the coefficients a,b,c,d ( y = a + b*x +c*x*x + d*x*x*x ) of the spline of
//the interval i. Returns true if i is a valid interval. These coefficients are for
//the spline in non-normalized coordinates, i.e. for xmin<x<xmax.
//
  const Double_t zero = 0.0;
  Int_t k;
  Double_t a0 = zero;
  Double_t a1 = zero;
  Double_t a2 = zero;
  Double_t a3 = zero;
  Double_t b0,b1;
  Bool_t inside = kFALSE;
  if ((i>=0) && (i<fN)) {
    inside = kTRUE;
    k  = fgU*i;
    a0 = fX[k];
    a1 = fX[k+1];
    a2 = fX[k+2];
    a3 = fX[k+3];
  }
  b0 = fCst;
  b1 = fSlope;
  a = a0 + a1*b0 + a2*b0*b0 +a3*b0*b0*b0;
  b = a1*b1 + 2*a2*b1*b0 + 3*a3*b1*b0*b0;
  c = a2*b1*b1 + 3*a3*b1*b1*b0;
  d = a3*b1*b1*b1;
  return inside;
}
Bool_t TSplineFit::GetSplineNN(Double_t x,Double_t &a,Double_t &b,Double_t &c,
                             Double_t &d) const {
// Give the coefficients a,b,c,d ( y = a + b*x +c*x*x + d*x*x*x ) of the spline of
//the interval containing x. Return true if x between xmin and xmax.  These
//coefficients are for the spline in non-normalized coordinates, i.e. for xmin<x<xmax.
//
  Bool_t inside;
  Int_t i,k;
  Double_t a0,a1,a2,a3,b0,b1;
  if ( x<fKhi[0] ) {
    inside = kFALSE;
    i=0;
  }
  else {
    if ( x >fKhi[fN] )  {
      inside = kFALSE;
      i = fN - 1;
    }
    else {
      inside = kTRUE;
      i = Interval(x);
    }
  }
  k  = fgU*i;
  a0 = fX[k];
  a1 = fX[k+1];
  a2 = fX[k+2];
  a3 = fX[k+3];
  b0 = fCst;
  b1 = fSlope;
  a = a0 + a1*b0 + a2*b0*b0 +a3*b0*b0*b0;
  b = a1*b1 + 2*a2*b1*b0 + 3*a3*b1*b0*b0;
  c = a2*b1*b1 + 3*a3*b1*b1*b0;
  d = a3*b1*b1*b1;
  return inside;
}
Bool_t TSplineFit::GetUpBound(Double_t &upbound) const {
// If true returned, then upbound returned is the upper bound, else upbound
//untouched
  if (fBoundedUp) upbound = fUpBound;
  return fBoundedUp;
}
Double_t TSplineFit::GetXLowInterval(Int_t i) const {
// If i is a valid sub-interval number, returns x at left of interval i, else
//returns -1.0e-99
  const Double_t bad = -1.0e-99;
  Double_t x;
  if ((i>=0) && (i<fN)) x = fKhi[i];
  else                  x = bad;
  return x;
}
Double_t TSplineFit::GetXUpInterval(Int_t i) const {
// If i is a valid sub-interval number, returns x at right of interval i, else
//returns +1.0e-99
  const Double_t bad = 1.0e-99;
  Double_t x;
  if ((i>=0) && (i<fN)) x = fKhi[i+1];
  else                  x = bad;
  return x;
}
void TSplineFit::Init() {
//Pointers to 0 and static constant variables to their constant value
  const Double_t zero = 0.0;
  if (!fgFits) InitStatic();
  fgCounter++;
  fType          = NotDefined;
  fCat           = -1;
  fNbInFamily    = -1;
  fM             = 0;
  fMi            = 0;
  fMl            = 0;
  fN             = 0;
  fNs2           = 0;
  fBoundedLow    = kFALSE;
  fLowBound      = zero;
  fBoundedUp     = kFALSE;
  fUpBound       = zero;
  fUseForRandom  = kFALSE;
  fParameter     = zero;
  fParameterDef  = "";
  fSource        = "";
  fMacro         = "";
  fXLabel        = "";
  fYLabel        = "";
  fZLabel        = "";
  fVLabel        = "";
  fMemoryReduced = kFALSE;
  fProvidedName  = "";
  f2Drestored    = kFALSE;
  f3Drestored    = kFALSE;
//pointers
  fZigZag        = 0;
  fHGenRandom    = 0;
  fHShowRandom   = 0;
  fInterpolation = 0;
  fSplineFitFunc = 0;
  fPointsGraph   = 0;
  fSplineGraph   = 0;
  fPS            = 0;
  fProvidedH1D   = 0;
  fProvidedH2D   = 0;
  fH2Dfit        = 0;
  fProvidedH3D   = 0;
  fH3Dfit        = 0;
//  Clear();
}
Bool_t TSplineFit::InitCatAndBounds(Int_t cat,Bool_t lowbounded,Double_t ylow,
                                    Bool_t upbounded,Double_t yup) {
//Setting up of category and bounds
  Bool_t ok = kTRUE;
  fCat        = cat;
  if ((fCat<0) || (fCat>99)) {
    ok = kFALSE;
    fCat = 99;
    std::cout << "TSplineFit::InitCatAndBounds: Error : 0 <= categories <=99  !" << std::endl;
    std::cout << "   category put to 99 !" << std::endl;
  }
  fBoundedLow = lowbounded;
  fLowBound   = ylow;
  fBoundedUp  = upbounded;
  fUpBound    = yup;
  return ok;
}
Bool_t TSplineFit::InitCheckBounds() {
// Checks data values with respect to bounds
  const Double_t big =  1.0e+99;
  const Double_t eps =  5.0e-3; //tolerance with respect to lower bound
  Int_t i;
  Double_t ymax,ymin,accept;
  Bool_t ok = kTRUE;
  if (fBoundedLow) {
    ymax = -big;
    ymin =  big;
    for (i=0;i<fM;i++) {
//If data below bound, we set it to the bound and issue a warning
      if (fMv[i]<fLowBound) {
        ok = kFALSE;
        fMv[i] = fLowBound;
        std::cout << "TSplinefit::InitCheckBounds : y data below lower bound" << std::endl;
        std::cout << "  data set to lower bound" << std::endl;
      }
      if (fMv[i]>ymax) ymax = fMv[i];
      if (fMv[i]<ymin) ymin = fMv[i];
    }
    accept = eps*(ymax-ymin);
//If data too close to the bound, add a small quantity to avoid infinities
    for (i=0;i<fM;i++) {
      if ((fMv[i]-fLowBound)<accept) fMv[i] = fLowBound + accept;
    }
  }
  return ok;
}
void TSplineFit::InitDimensions(Int_t M,Int_t m) {
//Setting up of dimensions
  Int_t i,nl,nc;
  fM   = M;
  fMi  = m;
// We refuse that the last interval contains only one point
  i    = M % m;
  switch (i) {
  case 0:
    fN = M/m;
    fMl = fMi;
    break;
  case 1:
    fN = M/m;
    fMl = fMi + 1;
    break;
  default:
    fN = M/m + 1;
    fMl = i;
    break;
  }
  fNs2 = 1;
  while (fNs2 < fN) {
    fNs2 *= 2;
  }
  if (fNs2>=fN) fNs2 /= 2;
  nl = fgU*fN - 3; //the 3 Lagrange parameters of the spline fN do not contribute
  nc = 2*fgM + 1;
  if (nc>nl) nc=nl;
  fA.ResizeTo(nl,nc);
  fB.ResizeTo(nl,1);
  fX.Set(nl);
  fKhi.Set(fN+1);
  fMt.Set(fM);
  fMv.Set(fM);
  fMs.Set(fM);
}
void TSplineFit::InitIntervals(Double_t xmin,Double_t xmax) {
// Finds the intervals containing one spline each.
  const Double_t x0n = -1.0;    //lower bound of first interval in normalized coordinates
  const Double_t x1n =  1.0;    //upper bound of last  interval in normalized coordinates
  Int_t i,k;
  if (xmin<xmax) {
    if (xmin<=fMt[0])    fKhi[0]  = xmin;
    else                 fKhi[0]  = 0.5*(3*fMt[0] - fMt[1]);
    if (xmax>=fMt[fM-1]) fKhi[fN] = xmax;
    else                 fKhi[fN] = 0.5*(3*fMt[fM-1] - fMt[fM-2]);
  }
  else {
    fKhi[0]  = 0.5*(3*fMt[0] - fMt[1]);
    fKhi[fN] = 0.5*(3*fMt[fM-1] - fMt[fM-2]);
  }
  for (k=1;k<fN;k++) {
    i       = k*fMi;
    fKhi[k] = 0.5*(fMt[i] + fMt[i-1]);
  }
  fSlope = (x1n - x0n)/(fKhi[fN] - fKhi[0]);
  fCst   = x0n - fKhi[0]*fSlope;
}
void TSplineFit::InitStatic() {
// Initialization of all static pointers
  if (!fgCat) fgCat = new TArrayI(100);
  if (!fgProgName)   NameProg();
  if (!fgWebAddress) NameWeb();
  if (!fgFileName)   NameFile();
  if (!fgFits) fgFits = new TObjArray();
}
Double_t TSplineFit::Integral(Double_t x0,Double_t x1) {
// Calculates integral over all splines between x0 and x1
  Bool_t ok;
  Short_t i;
  Double_t a0,a1,a2,a3,xmin,xmax,xx0,xx1;
  Double_t S = 0.0;
  TPoly3 p3;
  for (i=0;i<fN;i++) {
    xmin = fKhi[i];
    xmax = fKhi[i+1];
    if ((x0<xmax) && (x1>xmin)) {
      ok = GetSplineNN(i,a0,a1,a2,a3);
      p3.Set(a0,a1,a2,a3);
      if (x0>xmin) xx0 = x0;
      else         xx0 = xmin;
      if (x1<xmax) xx1 = x1;
      else         xx1 = xmax;
      S += p3.Integral(xx0,xx1);
    }
  }
  return S;
}
Int_t TSplineFit::Interpolation() {
// This fit is not a fit! It is a spline interpolation. We use TSpline3.
  Int_t ifail = 0;
  TArrayD A(fM);
  if (fBoundedLow) {
    ApplyLowBound(A);
    fInterpolation = new TSpline3(GetTitle(),fMt.GetArray(),A.GetArray(),fM);
  }
  else fInterpolation = new TSpline3(GetTitle(),fMt.GetArray(),fMv.GetArray(),fM);
  return ifail;
}
Int_t TSplineFit::Interval(Double_t x) const {
//  Finds the interval containing x.
//  If outside bound from the left,  returns 1st  interval. Beware that extrapolating a
//spline may give non-sense results.
//  If outside bound from the right, returns last interval. Beware that extrapolating a
//spline may give non-sense results
  Int_t imin,imid,imax,iter;
  Int_t jmin,jmid,jmax;
  Double_t xmin,xmax,xmid;
  Int_t itv;
  imin = 0;
  jmin = 0;
  imid = fNs2;
  jmid = fNs2;
  iter = fNs2;
  imax = 2*fNs2;
  jmax = fN;
  xmin = fKhi[0];
  xmid = fKhi[imid];
  xmax = fKhi[fN];
  if (x<=xmin) return 0;
  if (x>=xmax) return fN-1;
  while ((imax-imin)>1) {
    iter /= 2;
    if (x<xmid) {
      imax  = imid;
      jmax  = imid;
      if (jmax > fN) jmax = fN;
      imid -= iter;
      jmid  = imid;
      if (jmid > fN) jmid = fN;
      xmax  = fKhi[jmax];
      xmid  = fKhi[jmid];
    }
    else {
      imin  = imid;
      jmin  = imin;
      if (jmin > fN) jmin = fN;
      imid += iter;
      jmid  = imid;
      if (jmid > fN) jmid = fN;
      xmin  = fKhi[jmin];
      xmid  = fKhi[jmid];
    }
  }
  itv = imin;
  return itv;
}
Bool_t TSplineFit::IsEqual(const TObject *obj) const {
//Do 2 TSplineFit are equal?
  Bool_t equal=kFALSE;
  TString s1,s2;
  s1 = GetName();
  s2 = ((TSplineFit *)obj)->GetName();
  if (fCat == ((TSplineFit *)obj)->fCat) {
    if (s1==s2) equal = kTRUE;
  }
  return equal;
}
Bool_t TSplineFit::IsInCollection() const {
// Returns true if "this" is in collection fgFits, false otherwise.
  TSplineFit *pfit;
  TIter next(fgFits);
  Bool_t found = kFALSE;
  while ((!found) && (pfit = (TSplineFit *)next()))  found = pfit==this;
  return found;
}
Bool_t TSplineFit::IsInCollection(TSplineFit *pf) {
// Returns true if fit pf is in collection fgFits, false otherwise.
  TSplineFit *pfit;
  if (!fgFits) fgFits = new TObjArray();
  TIter next(fgFits);
  Bool_t found = kFALSE;
  while ((!found) && (pfit = (TSplineFit *)next()))  found = pfit==pf;
  return found;
}
Int_t TSplineFit::LoadFamily() const {
// Load all members of the family from the database file into the collection, if they are
//not already loaded.
//The number of elements in the family is returned.
// This method has to be called with any member of the family
  Int_t N = 0;
  if (fNbInFamily>=0) {
    TString s;
    Int_t M;
    Bool_t finished = kFALSE;
    TSplineFit *fit = 0;
    s = GetName();
    M = s.Length();
    s.Resize(M-5);
    AddNumbering(N,s);
    while (!finished) {
      fit = FindFit(s.Data());
      if (fit) {
        N++;
        M = s.Length();
        s.Resize(M-5);
        AddNumbering(N,s);
      }
      else finished = kTRUE;
    }
  }
  else std::cout << "TSplineFit::LoadFamily ERROR LoadFamily called with fit without family" << std::endl;
  return N;
}Int_t TSplineFit::LoadFamily(const Text_t *family) {
// In case family is the name of a family of fits, load all members of the
//family from the database file into the collection, if they are not already loaded.
//The number of elements in the family is returned.
  TString s;
  Int_t M;
  Int_t N = 0;
  Bool_t finished = kFALSE;
  TSplineFit *fit = 0;
  s = family;
  AddNumbering(N,s);
  while (!finished) {
    fit = FindFit(s.Data());
    if (fit) {
      N++;
      M = s.Length();
      s.Resize(M-5);
      AddNumbering(N,s);
    }
    else finished = kTRUE;
  }
  return N;
}
void TSplineFit::MinMax(Bool_t &ismin,Double_t &xmin,Double_t &ymin,
                        Bool_t &ismax,Double_t &xmax,Double_t &ymax) const  {
// Finds the lowest among the minima of the fitted distribution, if there is at least
//one minimum, and the biggest among the maxima of the distribution, if there is at least
//one maximum.
// Important Notice: this method cannot be applied when you have asked for a lower bound
//for the fit, or when you have asked for an interpolation instead of a fit.
//Do not call it in this case!
  Int_t i;
  Bool_t ismintmp,ismaxtmp;
  Double_t xmintmp,ymintmp,xmaxtmp,ymaxtmp;
  Bool_t firstmin = kTRUE;
  Bool_t firstmax = kTRUE;
  ismin = kFALSE;
  ismax = kFALSE;
  if (!fBoundedLow) {
    for (i=0;i<fN;i++) {
      MinMax(i,ismintmp,xmintmp,ymintmp,ismaxtmp,xmaxtmp,ymaxtmp);
      if (ismintmp) {
        if (firstmin) {
          firstmin = kFALSE;
          ismin    = kTRUE;
          xmin     = xmintmp;
          ymin     = ymintmp;
        }
        else {
          if (ymintmp<ymin) {
            xmin     = xmintmp;
            ymin     = ymintmp;
          }//end if (ymintmp<ymin)
        }//end else if (firstmin)
      }//end if (ismintmp)
      if (ismaxtmp) {
        if (firstmax) {
          firstmax = kFALSE;
          ismax    = kTRUE;
          xmax     = xmaxtmp;
          ymax     = ymaxtmp;
        }
        else {
          if (ymaxtmp>ymax) {
            xmax     = xmaxtmp;
            ymax     = ymaxtmp;
          }//end if (ymaxtmp>ymax)
        }//end else if (firstmax)
      }//end if (ismaxtmp)
    }//end for (i=0;i<fN;i++)
  }//end if (!fBoundedLow)
  else {
    std::cout << "TSplineFit::MinMax : Do not call MinMax for a fit with a lower bound" << std::endl;
    std::cout << "  this method is not working in case of lower bound" << std::endl;
  }//end else if (!fBoundedLow)
}
void TSplineFit::MinMax(Int_t i,Bool_t &ismin,Double_t &xmin,Double_t &ymin,
                        Bool_t &ismax,Double_t &xmax,Double_t &ymax) const  {
// For interval i, finds if there is a minimum or a maximum inside the interval.
//  minimum in mathematical sense : point where y' = 0 and y" < 0
//  maximum in mathematical sense : point where y' = 0 and y" > 0
// Important Notice: this method cannot be applied when you have asked for a lower bound
//for the fit. Do not call it in this case!
  const Double_t zero = 0.0;
  const Double_t eps  = 1.0e-20;
  Bool_t ok;
  Double_t a,b,c,d,A,B,C,D,x1,x2,yss;
  ismin = kFALSE;
  ismax = kFALSE;
  if (!fBoundedLow) {
    ok = GetSplineNN(i,a,b,c,d);
    if (ok) {
      A = 3*d;
      B = c;
      C = b;
      D = B*B - A*C;
      if (D>=zero) {
        D = TMath::Sqrt(D);
        x1 = (-B+D)/A;
        x2 = (-B-D)/A;
        if ((x1>=fKhi[i]) && (x1<fKhi[i+1])) {
          yss = c + 3*d*x1;
          if (TMath::Abs(yss)>eps) {
            if (yss<zero) {
              ismax = kTRUE;
              xmax  = x1;
              ymax  = a + b*x1 + c*x1*x1 + d*x1*x1*x1;
            }
            else {
              ismin = kTRUE;
              xmin  = x1;
              ymin  = a + b*x1 + c*x1*x1 + d*x1*x1*x1;
            }//end else if (yss>zero)
          }//end if (TMath::Abs(yss)>eps)
        }//end ((x1>=fKhi[i]) && (x1<(fKhi[i+1]))
        if ((x2>=fKhi[i]) && (x2<fKhi[i+1])) {
          yss = c + 3*d*x2;
          if (TMath::Abs(yss)>eps) {
            if (yss<zero) {
              ismax = kTRUE;
              xmax  = x2;
              ymax  = a + b*x2 + c*x2*x2 + d*x2*x2*x2;
            }
            else {
              ismin = kTRUE;
              xmin  = x2;
              ymin  = a + b*x2 + c*x2*x2 + d*x2*x2*x2;
            }//end else if (yss>zero)
          }//end if (TMath::Abs(yss)>eps)
        }//end if ((x2>=fKhi[i]) && (x2<(fKhi[i+1]))
      }//end if (D>=zero)
    }//end if (ok)
  }//end if (!fBoundedLow)
  else {
    std::cout << "TSplineFit::MinMax : Do not call MinMax for a fit with a lower bound" << std::endl;
    std::cout << "  this method is not working in case of lower bound" << std::endl;
  }//end else if (!fBoundedLow)
}
void TSplineFit::MultinomialAsWeight(TH1 *h) {
// MultinomialAsWeight() replaces all the errors in the histogram h by the errors obtained
//taking the multinomial distribution as a weight function for the epsi (probability of
//falling into bin i). Advantage: no error 0, no weight infinite. A fit can be done with
//the errors calculated like this.
  const Double_t  un  = 1.0;
  const Double_t deux = 2.0;
  Int_t i;
  Double_t M,N,n,E;
  M = (Double_t)h->GetNbinsX();
  N = (Double_t)h->GetEntries();
  for (i=1;i<=M;i++) {
    n = h->GetBinContent(i);
    E = (n+un)*(un-(n+deux)/(N+M+un));
    h->SetBinError(i,E);
  }
}
void TSplineFit::NameFile(TString name) {
// Give a name to this program
  if (!fgFileName) fgFileName = new TString(name);
}
void TSplineFit::NameProg(TString name) {
// Give a name to this program
  if (!fgProgName) fgProgName = new TString(name);
}
void TSplineFit::NameWeb(TString name) {
// Give a name to this program
  if (!fgWebAddress) fgWebAddress = new TString(name);
}

/*
void TSplineFit::NameFile(Text_t *name) {
// Give a name to this program
  if (!fgFileName) fgFileName = new TString(name);
}
void TSplineFit::NameProg(Text_t *name) {
// Give a name to this program
  if (!fgProgName) fgProgName = new TString(name);
}
void TSplineFit::NameWeb(Text_t *name) {
// Give a name to this program
  if (!fgWebAddress) fgWebAddress = new TString(name);
}*/


Bool_t TSplineFit::OrderFile(Bool_t destroycopy) {
// rewrites the file with all its elements ordered. If destroycopy
//is true, the copied database file with suffix .rdbold is destroyed, else not.
  const Int_t    bufsize = 64000;
  const Text_t *win32gdk = "win32gdk";
  Bool_t FirstCat = kTRUE;
  Bool_t CatThere[100];
  Int_t i,nb,N,icat;
  TString s,s2,s3;
  TFile *lFitFile;
  TTree *lFitTree;
  TBranch *lFitBranch;
  TSplineFit *fit,*pfit;
  if (!fgFileName) NameFile();
  if (!fgFits) fgFits = new TObjArray();
  TIter next(fgFits);
  for (icat=0;icat<100;icat++) CatThere[icat] = kFALSE;
  s = *fgFileName;
  s.Append("old");
//First make a copy of the file
  fgFitFile = new TFile(fgFileName->Data(),"READ");
  if (fgFitFile->IsOpen()) {
    fgFitTree = (TTree *)fgFitFile->Get("AllFits");
    if (fgFitTree) {
      fit = new TSplineFit();
      fgFitBranch = fgFitTree->GetBranch("Fits");
      fgFitBranch->SetAddress(&fit);
      N = (Int_t)fgFitTree->GetEntries();
      lFitFile = new TFile(s.Data(),"RECREATE");
      lFitTree = new TTree("AllFits","AllFits");
      lFitBranch = lFitTree->Branch("Fits","TSplineFit",&fit,bufsize,0);
      nb = 0;
      i  = 0;
      while (i<N) {
        nb += fgFitTree->GetEntry(i);
        CatThere[fit->fCat] = kTRUE;
        lFitTree->Fill();
        i++;
      }//end while (i<N)
      lFitFile->Write();
      lFitFile->Close();
      delete lFitFile;
      lFitFile    = 0;
      lFitTree    = 0;
      lFitBranch  = 0;
      delete fit;
      fit = 0;
    }//end if (fgFitTree)
    else {
      fgFitFile->Close();
      delete fgFitFile;
      return kFALSE;
    }//end else if (fgFitTree)
  }//end if (fgFitFile->IsOpen())
  else {
    fgFitFile->Close();
    delete fgFitFile;
    return kFALSE;
  }//end else if (fgFitFile->IsOpen())
  fgFitFile->Close();
  delete fgFitFile;
  fgFitFile   = 0;
  fgFitTree   = 0;
  fgFitBranch = 0;
// Then take each category in turn, put all element of category icat into the collection,
//order them and copy them to the new file
  for (icat=0;icat<100;icat++) {
    if (CatThere[icat]) {
      fgFits->Delete();
      lFitFile = new TFile(s.Data(),"READ");
      if (lFitFile->IsOpen()) {
        lFitTree = (TTree *)lFitFile->Get("AllFits");
        if (lFitTree) {
          fit = new TSplineFit();
          lFitBranch = lFitTree->GetBranch("Fits");
          lFitBranch->SetAddress(&fit);
          N = (Int_t)lFitTree->GetEntries();
          nb = 0;
          i  = 0;
          while (i<N) {
            nb += lFitTree->GetEntry(i);
            if (fit->fCat==icat) {
              pfit = new TSplineFit(*fit);
              AddFit(pfit);
            }//end if (fit->fCat==icat)
            i++;
          }//end while (i<N)
          delete fit;
          fit = 0;
        }//end if (lFitTree)
        else {
          lFitFile->Close();
          delete lFitFile;
          return kFALSE;
        }//end else if (lFitTree)
      }//end if (lFitFile->IsOpen())
      else {
        lFitFile->Close();
        delete lFitFile;
        return kFALSE;
      }//end else if (lFitFile->IsOpen())
      lFitFile->Close();
      delete lFitFile;
      lFitFile   = 0;
      lFitTree   = 0;
      lFitBranch = 0;
//
      if (FirstCat) {
        FirstCat = kFALSE;
        fgFitFile = new TFile(fgFileName->Data(),"RECREATE");
        if (fgFitFile->IsOpen()) {
          fgFitTree = new TTree("AllFits","AllFits");
          fit = new TSplineFit();
          fgFitBranch = fgFitTree->Branch("Fits","TSplineFit",&fit,bufsize,0);
          next.Reset();
          while ((pfit = (TSplineFit *)next())) {
            fit = pfit;
            fgFitTree->Fill();
          }//end while (pfit = (TSplineFit *)next())
          fgFitFile->Write();
        }//end if (fgFitFile->IsOpen())
        else {
          fgFitFile->Close();
          delete fgFitFile;
          return kFALSE;
        }//end else if (fgFitFile->IsOpen())
        fgFitFile->Close();
        delete fgFitFile;
        fgFitFile   = 0;
        fgFitTree   = 0;
        fgFitBranch = 0;
      }//end if (FirstCat)
      else {
        fgFitFile = new TFile(fgFileName->Data(),"UPDATE");
        if (fgFitFile->IsOpen()) {
          fgFitTree = (TTree *)fgFitFile->Get("AllFits");
          if (fgFitTree) {
            fit = new TSplineFit();
            fgFitTree->SetBranchAddress("Fits",&fit);
            next.Reset();
            while ((pfit = (TSplineFit *)next())) {
              fit = pfit;
              fgFitTree->Fill();
            }//end while (pfit = (TSplineFit *)next())
            fgFitTree->Write("",TObject::kOverwrite);
          }//end if (fgFitTree)
          else {
            fgFitFile->Close();
            delete fgFitFile;
            return kFALSE;
          }
        }//end if (fgFitFile->IsOpen())
        else {
          fgFitFile->Close();
          delete fgFitFile;
          return kFALSE;
        }//end else if (fgFitFile->IsOpen())
        fgFitFile->Close();
        delete fgFitFile;
        fgFitFile   = 0;
        fgFitTree   = 0;
        fgFitBranch = 0;
      }//end else if (FirstCat)
    }//end if (CatThere[icat])
  }//end for (icat=0;icat<100;icat++)
  if (destroycopy) {
    s2 = gSystem->GetBuildArch();
    if (!s2.CompareTo(win32gdk)) {
      s3 = ".!del /q ";
      s3.Append(s.Data());
    }
    else {
      s3 = ".!rm -f ";
      s3.Append(s.Data());
    }
    gROOT->ProcessLine(s3.Data());
  }
  return kTRUE;
}
Double_t TSplineFit::Pedestal(Double_t xmin,Double_t xmax) {
// This method has only a sense in case the fitted curve is a pulse and the region
//between xmin and xmax is a region where the pulse has not yet started and is a
//good measure of the pedestal.
  Double_t S,ped;
  S = Integral(xmin,xmax);
  ped = S/(xmax-xmin);
  return ped;
}
void TSplineFit::Print() {
// Print all infos on this fit
  std::cout << std::endl;
  std::cout << "      " << GetTitle() << std::endl;
  std::cout << std::endl;
  std::cout << fSource.Data() << std::endl;
  std::cout << std::endl;
  std::cout << "Name of the fit        : " << GetName() << std::endl;
  std::cout << "Type of the fit        : " << fType << std::endl;
  std::cout << "Category               : " << fCat << std::endl;
  std::cout << "Family number          : " << fNbInFamily << std::endl;
  std::cout << "Nb. of measurements    : " << fM << std::endl;
  std::cout << "Nb. meas/spline        : " << fMi << std::endl;
  std::cout << "Nb. meas. last spline  : " << fMl << std::endl;
  std::cout << "Nb. of splines         : " << fN << std::endl;
  std::cout << "Slope    for [-1,1]    : " << fSlope << std::endl;
  std::cout << "Constant for [-1,1]    : " << fCst << std::endl;
  std::cout << "Has a lower bound      : " << fBoundedLow;
  if (fBoundedLow) std::cout << "    " << fLowBound;
  std::cout << std::endl;
  std::cout << "Has an upper bound     : " << fBoundedUp;
  if (fBoundedUp) std::cout << "    " << fUpBound;
  std::cout << std::endl;
  std::cout << "Xmin                   : " << fKhi[0] << std::endl;
  std::cout << "Xmax                   : " << fKhi[fN] << std::endl;
  std::cout << "Used for random gen.   : " << fUseForRandom << std::endl;
  std::cout << "Associated parameter   : " << fParameter << std::endl;
  std::cout << "Parameter definition   : " << fParameterDef.Data() << std::endl;
  std::cout << "Creation date          : " << fDate.Data() << std::endl;
  std::cout << "CINT macro of this fit : " << fMacro.Data() << std::endl;
  std::cout << "X axis                 : " << fXLabel.Data() << std::endl;
  if ((fType==SplineFit2D) || (fType==SplineFit3D))
    std::cout << "Y axis                 : " << fYLabel.Data() << std::endl;
  if (fType==SplineFit3D)
    std::cout << "Z axis                 : " << fZLabel.Data() << std::endl;
  std::cout << "Values axis            : " << fVLabel.Data() << std::endl;
  std::cout << "Memory reduced         : " << fMemoryReduced << std::endl;
  std::cout << "Chi2 of fit            : " << Chi2() << std::endl;
  std::cout << std::endl;
}
void TSplineFit::Purge() {
// The collection fgFits is purged
  if (!fgFits) fgFits = new TObjArray();
  fgFits->Delete();
  fgNextDraw = 0;
}
void TSplineFit::PurgeStatic() {
// delete all static pointers
  if (fgCat) {
    delete fgCat;
    fgCat = 0;
  }
  if (fgFits) {
    Purge();
    delete fgFits;
    fgFits = 0;
  }
  if (fgProgName) {
    delete fgProgName;
    fgProgName = 0;
  }
  if (fgWebAddress) {
    delete fgWebAddress;
    fgWebAddress = 0;
  }
  if (fgFileName) {
    delete fgFileName;
    fgFileName = 0;
  }
  if (fgFitFile) {
    fgFitFile->Close();
    delete fgFitFile;
    fgFitFile   = 0;
    fgFitTree   = 0;
    fgFitBranch = 0;
  }
}
Int_t TSplineFit::RedoFit(Bool_t debug) {
// Do the spline fit again, because something has changed, for instance the errors fMs[i].
//debug is for having debug printing.
  Int_t  ifail = -9;
  if (fMemoryReduced) {
    std::cout << "TSplineFit::RedoFit WARNING you cannot redo the fit" << std::endl;
    std::cout << "  Memory has been reduced: measurements are lost" << std::endl;
  }//end if (fMemoryReduced)
  else {
    switch (fType) {
      case NotDefined:
        std::cout << "TSplineFit::RedoFit ERROR type of fit undefined" << std::endl;
        break;
      case LinInterpol:
      case SplineInterpol:
        std::cout << "TSplineFit::RedoFit WARNING no fit to redo" << std::endl;
        std::cout << GetName() << " is an interpolation, not a fit" << std::endl;
        break;
      case SplineFit1D:
      case SplineFit2D:
      case SplineFit3D:
        Bool_t ok;
        ok = InitCheckBounds();
        InitIntervals(fKhi[0],fKhi[fN]);
        if (fMi<=1) std::cout << "TSplineFit::RedoFit ERROR fMi cannot be 1" << std::endl;
        else {
          Fill();
          ifail = Solve(debug);
        }
        if (ifail) {
          std::cout << "TSplineFit::RedoFit Problem in finding solution" << std::endl;
          std::cout << "   ifail = " << ifail << std::endl;
        }
        break;
    }//end switch (fType)
  }//end else if (fMemoryReduced)
  return ifail;
}
void TSplineFit::ReduceMemory() {
// Reduce the memory occupied up to the point of containing just what is needed in order
//to be able to call method V(). The fit remains there, but everything else, including
//the measurements fMt,fMv,fMs is withdrawn. The histograms fHGenRandom and fHShowRandom
//are not touched.
// Please do not save a fit onto the file fgFitFile AFTER having called ReduceMemory!
//Save it first, and then call ReduceMemory if you want.
  fA.ResizeTo(1,1);
  fB.ResizeTo(1,1);
  ClearGraphs(kFALSE);
  switch (fType) {
    case NotDefined:
      break;
    case LinInterpol:
      break;
    case SplineInterpol:
    case SplineFit1D:
    case SplineFit2D:
    case SplineFit3D:
      fMt.Set(1);
      fMv.Set(1);
      fMs.Set(1);
      fMemoryReduced = kTRUE;
      break;
  }
}
void TSplineFit::Regenerate() {
// Regenerate histos fHGenRandom and fHShowRandom in case fit used for random.
//Does not regenerate fInterpolation, since we save it now onto the file.
//case fit is an interpolation.
  if (fUseForRandom) UseForRandom();
  if (fType==SplineInterpol) Interpolation();
}
void TSplineFit::RemoveDisplay() {
// Delete display
  if (gOneDisplay) {
    delete gOneDisplay;
    gOneDisplay = 0;
  }
}
Bool_t TSplineFit::RemoveFitFromFile(Text_t *name,Bool_t destroycopy) {
// Remove fit named "name" from the file. If not found, returns false. If destroycopy
//is true, the copied database file with suffix .rdbold is destroyed, else not.
  const Int_t    bufsize = 64000;
  const Text_t *win32gdk = "win32gdk";
  Int_t i,nb,N;
  TString s,s2,s3;
  TFile *lFitFile;
  TTree *lFitTree;
  TBranch *lFitBranch;
  Bool_t ok = kFALSE;
  TSplineFit *fit;
  if (!fgFileName) NameFile();
  if (!fgFits) fgFits = new TObjArray();
  s = *fgFileName;
  s.Append("old");
  fit = FindFit(name,-1,kFALSE);
  if (fit) {
    fgFits->Remove(fit);
    fit = FindFit(name,-1,kFALSE);
    if (fit) {
//First make a copy of the file
      delete fit;
      fit = 0;
      fgFitFile = new TFile(fgFileName->Data(),"READ");
      fgFitTree = (TTree *)fgFitFile->Get("AllFits");
      fit = new TSplineFit();
      fgFitBranch = fgFitTree->GetBranch("Fits");
      fgFitBranch->SetAddress(&fit);
      N = (Int_t)fgFitTree->GetEntries();
      lFitFile = new TFile(s.Data(),"RECREATE");
      lFitTree = new TTree("AllFits","AllFits");
      lFitBranch = lFitTree->Branch("Fits","TSplineFit",&fit,bufsize,0);
      nb = 0;
      i  = 0;
      while (i<N) {
        nb += fgFitTree->GetEntry(i);
        lFitTree->Fill();
        i++;
      }//end while (i<N)
      lFitFile->Write();
      lFitFile->Close();
      delete lFitFile;
      lFitFile    = 0;
      lFitTree    = 0;
      lFitBranch  = 0;
      fgFitFile->Close();
      delete fgFitFile;
      fgFitFile   = 0;
      fgFitTree   = 0;
      fgFitBranch = 0;
      delete fit;
      fit = 0;
//Then recreate the database file from the copy, omitting "name"
      Bool_t thisone;
      lFitFile = new TFile(s.Data(),"READ");
      lFitTree = (TTree *)lFitFile->Get("AllFits");
      fit = new TSplineFit();
      lFitBranch = lFitTree->GetBranch("Fits");
      lFitBranch->SetAddress(&fit);
      N = (Int_t)lFitTree->GetEntries();
      fgFitFile = new TFile(fgFileName->Data(),"RECREATE");
      fgFitTree = new TTree("AllFits","AllFits");
      fgFitBranch = fgFitTree->Branch("Fits","TSplineFit",&fit,bufsize,0);
      nb = 0;
      i  = 0;
      while (i<N) {
        nb += lFitTree->GetEntry(i);
        s2   = fit->GetName();
        if (!s2.CompareTo(name)) {
          ok = kTRUE;
          thisone = kTRUE;
        }
        else thisone = kFALSE;
        if (!thisone) fgFitTree->Fill();
        i++;
      }//end while (i<N)
      fgFitFile->Write();
      fgFitFile->Close();
      delete fgFitFile;
      fgFitFile    = 0;
      fgFitTree    = 0;
      fgFitBranch  = 0;
      lFitFile->Close();
      delete lFitFile;
      lFitFile   = 0;
      lFitTree   = 0;
      lFitBranch = 0;
      delete fit;
      fit = 0;
    }//end if (fit)
  }//end if (fit)
  if (ok && destroycopy) {
    s2 = gSystem->GetBuildArch();
    if (!s2.CompareTo(win32gdk)) {
      s3 = ".!del /q ";
      s3.Append(s.Data());
    }
    else {
      s3 = ".!rm -f ";
      s3.Append(s.Data());
    }
    gROOT->ProcessLine(s3.Data());
  }
  return ok;
}
Bool_t TSplineFit::RestoreHisto() {
// When 2D fit read from file, fProvidedH2D or fProvidedH3D have to be restored
//from fZigZag, fMv and fMs.
  Bool_t ok;
  Int_t nx,ny,nz,i,j,m,k,kz;
  Double_t xmin,xmax,ymin,ymax,zmin,zmax;
  switch (fType) {
    case SplineFit2D:
      nx   = fZigZag->GetNx();
      xmin = fZigZag->GetXmin();
      xmax = fZigZag->GetXmax();
      ny   = fZigZag->GetNy();
      ymin = fZigZag->GetYmin();
      ymax = fZigZag->GetYmax();
      fProvidedH2D = new TH2D(GetName(),GetTitle(),nx,xmin,xmax,ny,ymin,ymax);
      for (j=1;j<=ny;j++) {
        for (i=1;i<=nx;i++) {
          k  = fProvidedH2D->GetBin(i,j);
          kz = fZigZag->NToZZ(i,j);
          fProvidedH2D->SetBinContent(k,fMv[kz]);
          fProvidedH2D->SetBinError(k,fMs[kz]);
        }
      }
      f2Drestored = kTRUE;
      ok = kTRUE;
      break;
    case SplineFit3D:
      nx   = fZigZag->GetNx();
      xmin = fZigZag->GetXmin();
      xmax = fZigZag->GetXmax();
      ny   = fZigZag->GetNy();
      ymin = fZigZag->GetYmin();
      ymax = fZigZag->GetYmax();
      nz   = fZigZag->GetNz();
      zmin = fZigZag->GetZmin();
      zmax = fZigZag->GetZmax();
      fProvidedH3D = new TH3D(GetName(),GetTitle(),nx,xmin,xmax,ny,ymin,ymax,nz,zmin,zmax);
      for (m=1;m<=nz;m++) {
        for (j=1;j<=ny;j++) {
          for (i=1;i<=nx;i++) {
            k  = fProvidedH2D->GetBin(i,j,m);
            kz = fZigZag->NToZZ(i,j,m);
            fProvidedH3D->SetBinContent(k,fMv[kz]);
            fProvidedH3D->SetBinError(k,fMs[kz]);
          }
        }
      }
      f3Drestored = kTRUE;
      ok = kTRUE;
      break;
    default:
      ok = kFALSE;
      break;
  }
  return ok;
}
void TSplineFit::SetDefaultLabels() {
// SetDefaultLabels() will replace the default labels of TOnePadDisplay (not
//specific to this fit) by the default labels specific to this fit.
// If gOneDisplay not yet booked, SetDefaultLabels will do it. The default labels are:
//
// - label1 : source of measurements for this fit
// - label2 : name of the CINT macro having produced this fit
// - label3 : "SplineFit" followed by the date of production of this fit
//
  if (!fgProgName) NameProg();
  if (!fgWebAddress) NameWeb();
  TString s = *fgProgName;
  s.Append("    ");
  s.Append(fDate);
  if (!gOneDisplay) {
    gOneDisplay = new TOnePadDisplay(fgProgName->Data(),fgWebAddress->Data(),
      fSource.Data(),fMacro.Data(),s.Data());
    gOneDisplay->BookCanvas();
  }
  else gOneDisplay->NewLabels(fSource.Data(),fMacro.Data(),s.Data());
}
void TSplineFit::SetMacro(Text_t *macro) {
// Set the name of the CINT macro having produced this fit
  fMacro = macro;
}
void TSplineFit::SetParameter(Double_t p, Text_t *t) {
// You have the possibility to associate with each fit a parameter fParameter and to
//place in fParameterDef a short explanation about what this parameter is. In fact,
//this parameter appears to be most useful in case of fits belonging to a family.
//Look at method BelongsToFamily() for this case.
  fParameter    = p;
  fParameterDef = t;
}
void TSplineFit::SetSource(Text_t *source) {
// You can give here the origin of the data for this fit
  fSource = source;
}
void TSplineFit::SetVLabel(Text_t *label) {
// Gives a title to the value axis. Valid for 1D, 2D and 3D fits. The value axis
//is the y axis for 1D, the z axis for 2D and the v axis for 3D !
  fVLabel = label;
}
void TSplineFit::SetXLabel(Text_t *label) {
// Gives a title to the X axis. Valid for 1D, 2D and 3D fits
  fXLabel = label;
}
void TSplineFit::SetYLabel(Text_t *label) {
// Gives a title to the Y axis. Valid only for 2D and 3D fits
  switch (fType) {
    case NotDefined:
      std::cout << "TSplineFit::SetYLabel ERROR fit is not defined" << std::endl;
      break;
    case LinInterpol:
    case SplineInterpol:
    case SplineFit1D:
      std::cout << "TSplineFit::SetYLabel WARNING replacing y axis by v axis!" << std::endl;
      fVLabel = label;
      break;
    case SplineFit2D:
    case SplineFit3D:
      fYLabel = label;
      break;
  }
}
void TSplineFit::SetZLabel(Text_t *label) {
// Gives a title to the Z axis. Valid only for 3D fits
  switch (fType) {
    case NotDefined:
      std::cout << "TSplineFit::SetZLabel ERROR fit is not defined" << std::endl;
      break;
    case LinInterpol:
    case SplineInterpol:
    case SplineFit1D:
    case SplineFit2D:
      std::cout << "TSplineFit::SetZLabel WARNING replacing z axis by v axis!" << std::endl;
      fVLabel = label;
      break;
    case SplineFit3D:
      fZLabel = label;
      break;
  }
}
void TSplineFit::ShowFitsInFile() {
// Prints list of names and titles of all fits in the database root file
  TString s1;
  Int_t nfit,nb,i;
  TSplineFit *fit;
  if (!fgFileName) NameFile();
  Int_t N = 0;
  std::cout << std::endl;
  std::cout << "    All fits in file" << std::endl;
  std::cout << std::endl;
  fgFitFile = new TFile(fgFileName->Data(),"READ");
  fgFitTree = (TTree *)fgFitFile->Get("AllFits");
  fit = new TSplineFit();
  fgFitBranch = fgFitTree->GetBranch("Fits");
  fgFitBranch->SetAddress(&fit);
  nfit = (Int_t)fgFitTree->GetEntries();
  nb   = 0;
  for (i=0;i<nfit;i++) {
    nb += fgFitTree->GetEntry(i);
    std::cout.width(30);
    std::cout.fill('.');
    std::cout.setf(std::ios::left,std::ios::adjustfield);
    std::cout << fit->GetName() << "     ";
    std::cout.width(60);
    std::cout.fill('.');
    std::cout.setf(std::ios::left,std::ios::adjustfield);
    std::cout << fit->GetTitle();
    std::cout << "    cat : "   << fit->fCat << std::endl;
    N++;
  }
  std::cout << std::endl;
  std::cout << "Nb. of fits : " << N << std::endl;
  std::cout << std::endl;
  delete fit;
  fit = 0;
  fgFitFile->Close();
  delete fgFitFile;
  fgFitFile   = 0;
  fgFitTree   = 0;
  fgFitBranch = 0;
}
void TSplineFit::ShowRandom() const {
//Shows the distribution of the random numbers generated
  fHShowRandom->Draw();
}
Int_t TSplineFit::Solve(Bool_t debug) {
// Finds the solution of the problem: all the splines and also the Lagrange parameters.
//Solution is found thanks to TBandedLE.
  const Double_t zero = 0.0;
  Int_t ifail;
  Int_t ndim,nrow,i;
  Double_t eps;
  TBandedLE ble(fA,fB,fgM);
  if (debug) {
    fA.Print();
    fB.Print();
  }
  ifail = ble.Solve();
  ndim = fX.fN;
  nrow = ble.fV.GetNrows();
  if (ndim != nrow) {
    std::cout << "TSplineFit::Solve ERROR fX has bad dimension" << std::endl;
    fX.Set(nrow);
  }
  if (!ifail) for (i=0;i<nrow;i++) fX[i] = ble.fV(i);
  else {
    for (i=0;i<nrow;i++) fX[i] = zero;
    std::cout << "TSplineFit::Solve : fit has failed !!!" << std::endl;
  }
  eps = ble.Verify();
  if (debug) std::cout << "TSplineFit::Solve  Verify : " << eps << std::endl;
  return ifail;
}
Bool_t TSplineFit::SolveLeft(Double_t &x,Double_t y0,Bool_t interval,
  Double_t xmin,Double_t xmax) {
// Finds the smallest x value for which y0 = f(x), where f(x) is the spline fit.
// In case interval is true, true is returned only if xmin <= x <= xmax.
// This method is useful for instance if the polynom is a rising signal, and one is interested
//in knowing where the signal reaches 10% or 90% of the pulse.
  Bool_t ok1;
  Bool_t ok = kFALSE;
  Short_t i;
  Double_t x0,x1,a0,a1,a2,a3;
  TPoly3 p3;
  i = 0;
  while ((!ok) && (i<fN)) {
    x0  = fKhi[i];
    x1  = fKhi[i+1];
    ok1 = GetSplineNN(i,a0,a1,a2,a3);
    p3.Set(a0,a1,a2,a3);
    ok = p3.SolveLeft(x,y0,kTRUE,x0,x1);
    i++;
  }
  if (interval) ok = ((x>=xmin) && (x<=xmax));
  return ok;
}
Bool_t TSplineFit::UpdateFile(Bool_t first) {
// Put this fit into the file.
// WARNING : first must be true ONLY when entering the first fit INTO a not yet
//existing database file. It must be false in all other cases! Putting it true if
//the database file already exists will destroy it and replace it with a database
//file containing just this last fit!
// The name or treename of the database file is contained in the static variable
//fgFileName which can be changed by the user.
//
  const Int_t bufsize = 64000;
  TSplineFit *fit;
  Bool_t ok = kFALSE;
  if (!fCat) {
    std::cout << "TSplineFit::UpdateFile WARNING:" << std::endl;
    std::cout << "  Fits of category 0 are not put into the database file" << std::endl;
    return ok;
  }
  if (first) {
    fgFitFile = new TFile(fgFileName->Data(),"RECREATE");
    if (fgFitFile) {
      ok = kTRUE;
      fgFitTree = new TTree("AllFits","AllFits");
      fit = this;
      fgFitBranch = fgFitTree->Branch("Fits","TSplineFit",&fit,bufsize,0);
      fgFitTree->Fill();
      fgFitFile->Write();
      fgFitFile->Close();
      delete fgFitFile;
      fgFitFile   = 0;
      fgFitTree   = 0;
      fgFitBranch = 0;
    }//end if (fgFitFile)
  }//end if (first)
  else {
    if (VerifyNotInFile()) {
      fgFitFile = new TFile(fgFileName->Data(),"UPDATE");
      fgFitTree = (TTree *)fgFitFile->Get("AllFits");
      if (fgFitTree) {
        ok = kTRUE;
        fit = this;
        fgFitTree->SetBranchAddress("Fits",&fit);
        fgFitTree->Fill();
        fgFitTree->Write("",TObject::kOverwrite);
      }
      fgFitFile->Close();
      delete fgFitFile;
      fgFitFile   = 0;
      fgFitTree   = 0;
      fgFitBranch = 0;
    }//end if (VerifyNotInFile())
    else ok = kTRUE;
  }//end else if (first)
  return ok;
}
Bool_t TSplineFit::UseForRandom(Bool_t usefr) {
// If the user wants to generate random numbers according to the fitted distribution,
//this is the method for allowing it. Notice that the condition is that the fitted
//distribution is never negative, so that the distribution MUST have been created
//with a positive lower bound.
// The method used is to create an histogram fHGenRandom whose bin content is according
//the fitted distribution and to use the Root method TH1::GetRandom(). The number of
//channels of this histogram is given by the static variable fgNChanRand, which has
//a default value of 128 and can be changed by the user before calling UseForRandom.
  const Double_t zero = 0.0;
  Bool_t ok = kFALSE;
  fUseForRandom = kFALSE;
  if (usefr) {
    TString s1,t1,s2,t2;
    switch (fType) {
      case NotDefined:
        std::cout << "TSplineFit::UseForRandom ERROR fit not defined" << std::endl;
        break;
      case LinInterpol:
      case SplineInterpol:
        std::cout << "TSplineFit::UseForRandom WARNING be sure that no value" << std::endl;
        std::cout << "  of the fit be negative in the whole range [xmin,xmax]" << std::endl;
        std::cout << "  in case of linear or spline interpolation" << std::endl;
      case SplineFit1D:
        Int_t i;
        Double_t x,y,w,ws2;
        ok = ((fType==LinInterpol) || (fType==SplineInterpol));
        ok = (ok || ((fType==SplineFit1D) && fBoundedLow && (fLowBound>=zero)));
        if (ok) {
          fUseForRandom = kTRUE;
          TDatime date;
          s1 = GetName();
          t1 = GetTitle();
          s2 = GetName();
          t2 = GetTitle();
          s1.Prepend("Gen_");
          t1.Prepend("Genenerator ");
          s2.Prepend("Show_");
          t2.Prepend("Show Genenerator ");
          s1.Append('_');
          s2.Append('_');
          s1 += date.GetTime();
          s2 += date.GetTime();
          fHGenRandom  = new TH1D(s1.Data(),t1.Data(),fgNChanRand,fKhi[0],fKhi[fN]);
          fHShowRandom = new TH1D(s2.Data(),t2.Data(),fgNChanRand,fKhi[0],fKhi[fN]);
          w   = (fKhi[fN] - fKhi[0])/fgNChanRand;
          ws2 = 0.5*w;
          x   = fKhi[0] + ws2;
          for (i=1;i<=fgNChanRand;i++) {
            y = TMath::Abs(V(x));
            fHGenRandom->SetBinContent(i,y);
            x += w;
          }
        }
        else std::cout << "TSplineFit::UseForRandom : ERROR : fit not bounded > 0" << std::endl;
        break;
      case SplineFit2D:
      case SplineFit3D:
        std::cout << "TSplineFit::UseForRandom ERROR 2D or 3D fits cannot be used" << std::endl;
        std::cout << "  for random number generation" << std::endl;
        break;
    }//end switch (fType)
  }//end if (usefr)
  else ok = kTRUE;
  if (!fUseForRandom) {
    if (fHGenRandom) {
      delete fHGenRandom;
      fHGenRandom = 0;
    }
    if (fHShowRandom) {
      delete fHShowRandom;
      fHShowRandom = 0;
    }
  }//end if (!fUseForRandom)
  return ok;
}
Double_t TSplineFit::V(Double_t x) const {
//  Find value y of the spline fit for abscissa x
  const Double_t z05 = 0.5;
  Int_t i,k;
  Double_t xn,x3;
  Double_t y=0.0;
  Double_t A,B,C,D; //coefficients of spline
  switch (fType) {
    case NotDefined:
      std::cout << "TSplineFit::V() ERROR type of fit not defined" << std::endl;
      break;
    case LinInterpol:
      Double_t x1,x2,y1,y2,m;
      i = Interval(x);
      k = i+1;
      x1 = fMt[i];
      if ((x1>x) && (i>0)) {
        i--;
        k--;
        x1 = fMt[i];
      }
      if (i>=(fM-1)) {
        i--;
        k--;
        x1 = fMt[i];
      }
      x2 = fMt[k];
      y1 = fMv[i];
      y2 = fMv[k];
      m  = (y2-y1)/(x2-x1);
      y  = y1 + m*(x-x1);
      break;
    case SplineInterpol:
      y = fInterpolation->Eval(x);
      if (fBoundedLow) {
        y = z05*(TMath::Sqrt(y*y + 4.0) + y + 2*fLowBound);
      }
      if ((fBoundedUp) && (y>fUpBound)) y = fUpBound;
      break;
    case SplineFit1D:
    case SplineFit2D:
    case SplineFit3D:
      i = Interval(x);
      k = fgU*i;
      A = fX[k];
      B = fX[k+1];
      C = fX[k+2];
      D = fX[k+3];
      xn = fSlope*x + fCst;
      x2 = xn*xn;
      x3 = xn*x2;
      y = A + B*xn + C*x2 + D*x3;
      if (fBoundedLow) {
        y = z05*(TMath::Sqrt(y*y + 4.0) + y + 2*fLowBound);
      }
      if ((fBoundedUp) && (y>fUpBound)) y = fUpBound;
      break;
 }
  return y;
}
Double_t TSplineFit::V(Double_t x,Double_t y) const {
// Value of the 2D fit at (x,y)
  Double_t yi;
  Double_t value = 0.0;
  if (fType==SplineFit2D) {
    Double_t v1,v2;
    Bool_t ok;
    TArrayD Yn;
    TArrayD Tn;
    ok = fZigZag->PointsNear(x,y,yi,Tn,Yn);
    if (ok) {
      v1 = V(Tn[0]);
      v2 = V(Tn[1]);
      value = v1 + ((v2-v1)/(Yn[1]-Yn[0]))*(yi-Yn[0]);
    }
    else std::cout << "TSplineFit::V(x,y) ERROR: nearest points not found" << std::endl;
  }
  else std::cout << "TSplineFit::V(x,y) ERROR: V(x,y) is only for 2D fits" << std::endl;
  return value;
}
Double_t TSplineFit::V(Double_t x,Double_t y,Double_t z) {
// Value of the 3D fit at (x,y,z)
  Double_t value = 0.0;
  if (fType==SplineFit3D) {
    Double_t t,v;
    Int_t i,n,kz;
    Bool_t ok;
    TArrayI I;
    TArrayD W;
    ok = fZigZag->NearestPoints(x,y,z,I,W);
    if (ok) {
      n = I.fN;
      for (i=0;i<n;i++) {
        kz = I[i];
        t  = fZigZag->T(kz);
        v  = V(t);
        value += W[i]*v;
      }
    }
    else std::cout << "TSplineFit::V(x,y,z) ERROR: nearest points not found" << std::endl;
  }
  else std::cout << "TSplineFit::V(x,y,z) ERROR: V(x,y) is only for 3D fits" << std::endl;
  return value;
}
Double_t TSplineFit::V(Int_t M,Double_t x,Double_t p) {
// Only for fits belonging to a family of fits.
// This method finds the 2 successive members of the family fit1 and fit2 for which
//
//  fit1->fParameter <= p < fit2->fParameter
//
// It calculates then a linear interpolation between the results at x of the fit1
//and of the fit2 fits.
// This method MUST be called with the first member of the family!
// M is the number of elements in the family, which you have got by a call to
//LoadFamily().
  const Double_t zero = 0.0;
  Double_t y = zero;
  if (fNbInFamily==0) {
    if (M==1) y=V(x);
    else {
      Double_t m,y1,y2,p1,p2;
      TSplineFit *fit1, *fit2;
      Bool_t found = kFALSE;
      Int_t i = 1;
      fit1 = this;
      p1   = fParameter;
      fit2 = (TSplineFit *)fgFits->After(fit1);
      p2   = fit2->fParameter;
      while ((!found) && (i<M)) {
        if (p<p2) found = kTRUE;
        else {
          fit1 = fit2;
          p1   = p2;
          fit2 = (TSplineFit *)fgFits->After(fit1);
          p2   = fit2->fParameter;
        }
        i++;
      }
      y1 = fit1->V(x);
      y2 = fit2->V(x);
      m  = (y2-y1)/(p2-p1);
      y  = y1 + m*(p-p1);
    }//end else if (M==1)
  }//end if (fNbInFamily==0)
  else {
    std::cout << "TSplineFit::V V for families must be called by the 1st member" << std::endl;
    std::cout << "  of the family" << std::endl;
  }//end else if (fNbInFamily==1)
  return y;
}
Bool_t TSplineFit::VerifyNotInFile() const {
// Verifies that this fit is not in the file
  TString s1,s2;
  Int_t nfit,nb,i;
  TSplineFit *fit;
  Bool_t found = kFALSE;
  s1 = GetName();
  fgFitFile = new TFile(fgFileName->Data(),"READ");
  if (fgFitFile->IsOpen()) {
    fgFitTree = (TTree *)fgFitFile->Get("AllFits");
    fit = new TSplineFit();
    fgFitBranch = fgFitTree->GetBranch("Fits");
    fgFitBranch->SetAddress(&fit);
    nfit = (Int_t)fgFitTree->GetEntries();
    nb   = 0;
    i    = 0;
    while ((!found) && (i<nfit)) {
      nb += fgFitTree->GetEntry(i);
      s2  = fit->GetName();
      if (!s1.CompareTo(s2)) found = kTRUE;
      i++;
    }
    delete fit;
    fit = 0;
    fgFitFile->Close();
    delete fgFitFile;
    fgFitFile   = 0;
    fgFitTree   = 0;
    fgFitBranch = 0;
  }
  return !found;
}
void TSplineFit::VerifyNT() {
//Verifies the 2 parts, category and fit of the name and title
  Bool_t already;
  Int_t N1,N2;
  TString sname,stitle,sn,st;
  TString sprefixn,sprefixt;
  sname    = GetName();
  stitle   = GetTitle();
  sn       = sname;
  st       = stitle;
  sprefixn = sname;
  sprefixt = stitle;
  already  = AlreadySeen(sprefixn,sprefixt);
  if (already) {
// Here an element of the same category has already been entered. We forget about the
//prefix given by the user, if he has given a prefix!
    if (sn.Contains("_")) {
// Here the user has given again a prefix. We kill it and replace it by the one already there
      N1 = sn.Index("_",1);
      N2 = sprefixn.Index("_",1);
      sname = sn.Replace(0,N1+1,sprefixn,N2+1);
    }// end if (sn.Contains("_")
    else {
// Here the user has not given a prefix. We add it.
      sname.Prepend(sprefixn);
    }//end else if (sn.Contains("_")
    if (st.Contains(" | ")) {
// Here the user has given again a prefix. We kill it and replace it by the one already there
      N1 = st.Index(" | ",3);
      N2 = sprefixt.Index(" | ",3);
      stitle = st.Replace(0,N1+3,sprefixt,N2+3);
    }//end if (st.Contains(" | ")
    else {
      stitle.Prepend(sprefixt);
    }//end else if (st.Contains(" | ")
  }//end if (already)
  else {
// Here it is the first element of this category which is entered. We have to verify
//that the user has provided the necessary prefix. If not, we have to provide a
//default one and issue a warning.
    if (!sn.Contains("_")) {
      sprefixn = "category";
      if (fCat<10) sprefixn.Append('0');
      sprefixn += fCat;
      sprefixn.Append('_');
      sname.Prepend(sprefixn);
    }//end else if (sn.Contains("_")
    if (!st.Contains(" | ")) {
      sprefixt = "category";
      if (fCat<10) sprefixt.Append('0');
      sprefixt += fCat;
      sprefixt.Append(" | ");
      stitle.Prepend(sprefixt);
    }
  }//end else if (already)
  SetName(sname.Data());
  SetTitle(stitle.Data());
}
Double_t TSplineFit::XpowerM(Double_t x,Int_t m) {
// Calculates x to the power m where m is an integer. This method is fast only for
//m small
  const Double_t un = 1.0;
  Int_t i;
  Double_t xm = un;
  if (m<0) {
    x = un/x;
    m = TMath::Abs(m);
  }
  if (m>0) for (i=0;i<m;i++) xm *= x;
  return xm;
}
Double_t SplineFitFunc(Double_t *x, Double_t *p) {
  Double_t xx,yy;
  xx = x[0];
  yy = gSplineFit->V(xx);
  return yy;
}
